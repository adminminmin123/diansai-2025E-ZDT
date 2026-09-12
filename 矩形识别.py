'''
实验名称：最小矩形检测与全角度圆形校正
实验平台：01Studio CanMV K230
教程：wiki.01studio.cc
说明：基于矩形主方向角精确校正透视变形，确保任意角度下圆形都显示为正圆
'''

import time, os, sys
import math
from media.sensor import *
from media.display import *
from media.media import *
from machine import UART, FPIOA
from ybUtils.YbUart import YbUart

# --------------------------- 硬件初始化 ---------------------------
# 串口初始化
#fpioa = FPIOA()
#fpioa.set_function(9, FPIOA.UART1_TXD)
#fpioa.set_function(10, FPIOA.UART1_RXD)
uart = YbUart(baudrate=115200)

# 屏幕分辨率设置
lcd_width = 640
lcd_height = 480

# 摄像头初始化
sensor = Sensor(width=1280, height=960)
sensor.reset()
sensor.set_framesize(width=640, height=480)
sensor.set_pixformat(Sensor.RGB565)

# 显示初始化
Display.init(Display.ST7701, width=lcd_width, height=lcd_height, to_ide=False)
MediaManager.init()
sensor.run()

# --------------------------- 配置参数 ---------------------------
thresholds = [(0, 70)]        # 二值化阈值
MIN_AREA = 1000               # 最小面积阈值
MAX_AREA = 100000             # 最大面积阈值
MIN_ASPECT_RATIO = 0.3        # 最小宽高比
MAX_ASPECT_RATIO = 3.0        # 最大宽高比

BASE_RADIUS = 45              # 基础半径（虚拟坐标单位）
POINTS_PER_CIRCLE = 24        # 增加采样点使圆形更平滑
PURPLE_THRESHOLD = (20, 60, 15, 70, -70, -20)  # 紫色色块阈值

# 基础矩形比例（可根据实际需求调整）
RECT_WIDTH = 210
RECT_HEIGHT = 95
TARGET_ASPECT_RATIO = RECT_WIDTH / RECT_HEIGHT  # 目标宽高比

CROSSHAIR_INTERVAL = 3000  # 3秒的毫秒数
last_crosshair_send_time = time.ticks_ms()  # 初始化上次发送时间
VIRTUAL_CENTER_INTERVAL = 5  # 发送间隔
virtual_center_counter = 0   # 帧计数器

# --------------------------- 工具函数 ---------------------------
def calculate_distance(p1, p2):
    return math.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2)

def calculate_center(points):
    if not points:
        return (0, 0)
    sum_x = sum(p[0] for p in points)
    sum_y = sum(p[1] for p in points)
    return (sum_x / len(points), sum_y / len(points))

def is_valid_rect(corners):
    edges = [calculate_distance(corners[i], corners[(i+1)%4]) for i in range(4)]

    ratio1 = edges[0] / max(edges[2], 0.1)
    ratio2 = edges[1] / max(edges[3], 0.1)
    valid_ratio = 0.5 < ratio1 < 1.5 and 0.5 < ratio2 < 1.5

    area = 0
    for i in range(4):
        x1, y1 = corners[i]
        x2, y2 = corners[(i+1) % 4]
        area += (x1 * y2 - x2 * y1)
    area = abs(area) / 2
    valid_area = MIN_AREA < area < MAX_AREA

    width = max(p[0] for p in corners) - min(p[0] for p in corners)
    height = max(p[1] for p in corners) - min(p[1] for p in corners)
    aspect_ratio = width / max(height, 0.1)
    valid_aspect = MIN_ASPECT_RATIO < aspect_ratio < MAX_ASPECT_RATIO

    return valid_ratio and valid_area and valid_aspect

def detect_purple_blobs(img):
    return img.find_blobs(
        [PURPLE_THRESHOLD],
        pixels_threshold=100,
        area_threshold=100,
        merge=True
    )

# 发送虚拟圆环点集
def send_circle_points(points):
    if not points:
        return
    count = len(points)
    msg = f"$$C,{count},"
    for x, y in points:
        msg += f"{x},{y},"
    msg = msg.rstrip(',') + "##"
    uart.write(msg)
    print(f"发送圆形点: {msg}")

# 发送激光准星中心点
def send_crosshair_position(cross_x,cross_y):
    msg = f"jiguang:({cross_x},{cross_y})"
    uart.write(msg)
    print(f"发送准星坐标: {msg}")

# 发送虚拟矩形中心点
def send_rect_center(center_x, center_y):
    # 按照要求格式封装坐标
    msg = f"origin:({center_x},{center_y})\r\n"
    # 通过串口发送
    uart.write(msg)
    # 打印调试信息
    print(f"发送矩形中心: {msg}")

def get_perspective_matrix(src_pts, dst_pts):
    """计算透视变换矩阵"""
    A = []
    B = []
    for i in range(4):
        x, y = src_pts[i]
        u, v = dst_pts[i]
        A.append([x, y, 1, 0, 0, 0, -u*x, -u*y])
        A.append([0, 0, 0, x, y, 1, -v*x, -v*y])
        B.append(u)
        B.append(v)

    # 高斯消元求解矩阵
    n = 8
    for i in range(n):
        max_row = i
        for j in range(i, len(A)):
            if abs(A[j][i]) > abs(A[max_row][i]):
                max_row = j
        A[i], A[max_row] = A[max_row], A[i]
        B[i], B[max_row] = B[max_row], B[i]

        pivot = A[i][i]
        if abs(pivot) < 1e-8:
            return None
        for j in range(i, n):
            A[i][j] /= pivot
        B[i] /= pivot

        for j in range(len(A)):
            if j != i and A[j][i] != 0:
                factor = A[j][i]
                for k in range(i, n):
                    A[j][k] -= factor * A[i][k]
                B[j] -= factor * B[i]

    return [
        [B[0], B[1], B[2]],
        [B[3], B[4], B[5]],
        [B[6], B[7], 1.0]
    ]

def transform_points(points, matrix):
    """应用透视变换将虚拟坐标映射到原始图像坐标"""
    transformed = []
    for (x, y) in points:
        x_hom = x * matrix[0][0] + y * matrix[0][1] + matrix[0][2]
        y_hom = x * matrix[1][0] + y * matrix[1][1] + matrix[1][2]
        w_hom = x * matrix[2][0] + y * matrix[2][1] + matrix[2][2]
        if abs(w_hom) > 1e-8:
            transformed.append((x_hom / w_hom, y_hom / w_hom))
    return transformed

def sort_corners(corners):
    """将矩形角点按左上、右上、右下、左下顺序排序"""
    center = calculate_center(corners)
    sorted_corners = sorted(corners, key=lambda p: math.atan2(p[1]-center[1], p[0]-center[0]))

    # 调整顺序为左上、右上、右下、左下
    if len(sorted_corners) == 4:
        # 找到最接近左上角的点
        left_top = min(sorted_corners, key=lambda p: p[0]+p[1])
        index = sorted_corners.index(left_top)
        sorted_corners = sorted_corners[index:] + sorted_corners[:index]
    return sorted_corners

def get_rectangle_orientation(corners):
    """计算矩形的主方向角（水平边与x轴的夹角）"""
    # 假设排序后的角点顺序为：左上、右上、右下、左下
    if len(corners) != 4:
        return 0

    # 计算上边和右边的向量
    top_edge = (corners[1][0] - corners[0][0], corners[1][1] - corners[0][1])
    right_edge = (corners[2][0] - corners[1][0], corners[2][1] - corners[1][1])

    # 选择较长的边作为主方向
    if calculate_distance(corners[0], corners[1]) > calculate_distance(corners[1], corners[2]):
        main_edge = top_edge
    else:
        main_edge = right_edge

    # 计算主方向角（弧度）
    angle = math.atan2(main_edge[1], main_edge[0])
    return angle

# --------------------------- 主循环 ---------------------------
clock = time.clock()
while True:
    clock.tick()
    img = sensor.snapshot()

    # 计算图像在LCD上的显示位置
    display_x = (lcd_width - sensor.width()) // 2
    display_y = (lcd_height - sensor.height()) // 2

    # 计算图像中心点（LCD坐标系）
    image_center_x = display_x + sensor.width() // 2
    image_center_y = display_y + sensor.height() // 2

    # 在图像中心绘制蓝色小十字准星（图像坐标系）
    img.draw_cross(image_center_x - display_x,
                  image_center_y - display_y,
                  color=(0, 0, 255),
                  thickness=2)

    # 发送准星坐标(3s发送一次)
    current_time = time.ticks_ms()
    if time.ticks_diff(current_time, last_crosshair_send_time) >= CROSSHAIR_INTERVAL:
        # 发送固定的激光坐标（320,240）- 屏幕中心
        send_crosshair_position(320, 240)
        last_crosshair_send_time = current_time

    # 图像处理
    gray_img = img.to_grayscale()
    binary_img = gray_img.binary(thresholds)
    binary_img.erode(1)
    binary_img.dilate(3)

    # 检测紫色色块
    purple_blobs = detect_purple_blobs(img)
    for blob in purple_blobs:
        img.draw_rectangle(blob[0:4], color=(255, 0, 255), thickness=1)
        img.draw_cross(blob.cx(), blob.cy(), color=(255, 0, 255), thickness=1)

    # 检测所有矩形并筛选出最小的一个
    min_area = float('inf')
    smallest_rect = None

    for r in binary_img.find_rects(threshold=12000):
        corners = r.corners()
        if is_valid_rect(corners):
            # 计算矩形面积
            area = 0
            for i in range(4):
                x1, y1 = corners[i]
                x2, y2 = corners[(i+1) % 4]
                area += (x1 * y2 - x2 * y1)
            area = abs(area) / 2

            # 更新最小矩形
            if area < min_area:
                min_area = area
                smallest_rect = corners

    # 只处理最小的矩形
    if smallest_rect:
        # 对矩形角点进行排序
        sorted_corners = sort_corners(smallest_rect)

        # 添加实际矩形中心计算
        center_x = int(sum(p[0] for p in sorted_corners) / 4)
        center_y = int(sum(p[1] for p in sorted_corners) / 4)

        # 绘制矩形边框和角点
        for i in range(4):
            x1, y1 = sorted_corners[i]
            x2, y2 = sorted_corners[(i+1) % 4]
            img.draw_line(x1, y1, x2, y2, color=(255, 0, 0), thickness=1)
        for p in sorted_corners:
            img.draw_circle(p[0], p[1], 5, color=(0, 255, 0), thickness=1)

        # 计算矩形主方向角
        angle = get_rectangle_orientation(sorted_corners)

        # 计算矩形实际宽高
        width = calculate_distance(sorted_corners[0], sorted_corners[1])
        height = calculate_distance(sorted_corners[1], sorted_corners[2])
        actual_aspect = width / max(height, 0.1)

        # 确定矩形是横向还是纵向（考虑旋转）
        is_horizontal = actual_aspect >= 1.0

        # 构建虚拟矩形（根据方向调整）
        if is_horizontal:
            virtual_rect = [
                (0, 0),
                (RECT_WIDTH, 0),
                (RECT_WIDTH, RECT_HEIGHT),
                (0, RECT_HEIGHT)
            ]
        else:
            virtual_rect = [
                (0, 0),
                (RECT_HEIGHT, 0),
                (RECT_HEIGHT, RECT_WIDTH),
                (0, RECT_WIDTH)
            ]

        # 计算校正半径（基于实际宽高比）
        if is_horizontal:
            radius_x = BASE_RADIUS
            radius_y = BASE_RADIUS / actual_aspect
        else:
            radius_x = BASE_RADIUS * actual_aspect
            radius_y = BASE_RADIUS

        # 计算虚拟矩形中心
        virtual_center = (RECT_WIDTH/2, RECT_HEIGHT/2) if is_horizontal else (RECT_HEIGHT/2, RECT_WIDTH/2)

        #这里添加了一个发送频率
        virtual_center_counter = (virtual_center_counter + 1) % VIRTUAL_CENTER_INTERVAL

        if virtual_center_counter  == 0 and center_x is not None and center_y is not None:
            send_rect_center(center_x, center_y)

        # 在虚拟矩形中生成椭圆点集（映射后为正圆）
        virtual_circle_points = []
        for i in range(POINTS_PER_CIRCLE):
            angle = 2 * math.pi * i / POINTS_PER_CIRCLE
            x = virtual_center[0] + radius_x * math.cos(angle)
            y = virtual_center[1] + radius_y * math.sin(angle)
            virtual_circle_points.append((x, y))


        # 计算透视变换矩阵并映射坐标
        matrix = get_perspective_matrix(virtual_rect, sorted_corners)
        if matrix:
            mapped_points = transform_points(virtual_circle_points, matrix)
            int_points = [(int(round(x)), int(round(y))) for x, y in mapped_points]

            # 绘制圆形
            for (x, y) in int_points:
                img.draw_circle(x, y, 2, color=(255, 0, 255), thickness=2)

            # 绘制圆心
            mapped_center = transform_points([virtual_center], matrix)
            if mapped_center:
                cx, cy = map(int, map(round, mapped_center[0]))
                img.draw_circle(cx, cy, 3, color=(0, 0, 255), thickness=3)

            # 发送坐标
            send_circle_points(int_points)

    # 显示FPS
    fps = clock.fps()
    #img.draw_string_advanced(10, 10, 20, f"FPS: {fps:.1f}", color=(255, 255, 255))

    # 显示图像
    Display.show_image(img,
                      x=round((lcd_width-sensor.width())/2),
                      y=round((lcd_height-sensor.height())/2))

    print(fps)
