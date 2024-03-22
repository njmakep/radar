import tkinter as tk
import math
import serial
import time #timeout을 주기 위해서

WIDTH = 640
HEIGHT = 480
angle = 0
direction = 0
sendingAngle = 0
objects = [[0,0],[10,0],[20,0],[30,0],[40,0],[50,0],[60,0],[70,0],[80,0],[90,0],
           [100,0],[110,0],[120,0],[130,0],[140,0],[150,0],[160,0],[170,0],[180,0]]

ser = serial.Serial("COM7",115200)
# 개체 호출
root = tk.Tk()
root.title("Ultrasonic Radar")
canvas = tk.Canvas(root, width=WIDTH, height=HEIGHT, bg = "black")
button = tk.Button(root, text = "Finish")
button.pack()
canvas.pack()

def drawObject(angle, distance):
    radius = WIDTH/2
    x=radius + math.cos(angle*math.pi/180)*distance
    y=radius - math.sin(angle*math.pi/180)*distance
    canvas.create_oval(x-5,y-5,x+5,y+5,fill='green') # 정사각형 안의 원

def updateScan():
    global angle # 전역변수를 함수 내에서 업데이트 하기 위해서 global 사용
    global direction
    global objects
    global sendingAngle

    receiveDistance = 0
    #각도 전송
    if angle % 10 == 0:
        sendingAngle = angle
        mask = b'\x7f' #byte로 mask 선언
        ser.write(bytes(bytearray([0x02, 0x52])))
        angleH = (angle>>7) + 128
        angleL = (angle & mask[0]) + 128
        crc = (0x02 + 0x52 + angleH + angleL) % 256
        ser.write(bytes(bytearray([angleH, angleL, crc, 0x03])))
    #거리수신
    if ser.in_waiting > 0:
        data = ser.read()
        if data == b'\x02':
            #두번째 바이트 수신 대기
            timeout = time.time() + 0.002
            lostData = False
            while ser.in_waiting < 5:
                # 타임아웃 처리
                if time.time() > timeout:
                    lostData = True
                    break
            if lostData == False:
                data = ser.read(5) #5개의 데이터를 한번에 읽어라
                if data[0] == 65:
                    # CRC 검사
                    crc = (2 + data[0] + data[1] + data[2]) % 256 #command 부터 data[0]이다. 주의!
                    if crc == data[3]:
                        if data[4] ==3:
                            #데이터 파싱
                            mask = b'\x7f'
                            data_one = bytes([data[1] & mask[0]])
                            receiveDistance = int.from_bytes(data_one) << 7
                            data_one = bytes([data[2] & mask[0]])
                            receiveDistance += int.from_bytes(data_one)
                            #물체 업데이트
                            for obj in objects:
                                if obj[0] == sendingAngle:
                                    obj[1] = receiveDistance
                        
    # 화면 지우기
    canvas.delete('all')
    #레이더 선 그리기
    radius = WIDTH / 2
    length = radius
    x= radius + math.cos(angle*math.pi/180)*length
    y= radius - math.sin(angle*math.pi/180)*length
    canvas.create_line(x,y,radius,radius,fill='green',width=4)
    #물체 그리기
    for obj in objects: #objects에 있는 것을 obj로 불러옴
        drawObject(obj[0], obj[1])
    #각도 업데이트
    if direction == 0:
        angle += 1
        if angle == 181:
            direction = 1
    else:
        angle -= 1
        if angle  == -1:
            direction = 0

    #angle = angle + 1
    #if angle > 359:
    #    angle = 0
    #x = 320 + math.cos(angle*math.pi/180)*200
    #y = 240 - math.sin(angle*math.pi/180)*200 #좌측 상단이 0,0으로 아래로 음수
    #canvas.create_line(320,240,x,y,fill='red',width=2)
    
    # 재귀호출
    canvas.after(50, updateScan)

# 그리기
#canvas.create_line(10,10,100,100,fill='red',width=2)

# 화면 표시
updateScan()
root.mainloop()
