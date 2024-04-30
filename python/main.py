import serial
import uinput

# ser = serial.Serial('/dev/ttyACM0', 115200, timeout=10)
ser = serial.Serial('/dev/rfcomm0', 115200, timeout=10)
ser.flushInput()



# Create new mouse device
device = uinput.Device([
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT,
    uinput.REL_X,
    uinput.REL_Y,
])

# Create new keyboard device
keyboard = uinput.Device([
    uinput.KEY_W,
    uinput.KEY_A,
    uinput.KEY_S,
    uinput.KEY_D,
    uinput.KEY_SPACE, #jump
    uinput.KEY_LEFTCTRL, #dive
    uinput.KEY_LEFTSHIFT, #grab
])


def parse_data(data):
    axis = data[1]  # 0 for X, 1 for Y
    value = int.from_bytes(data[2:4], byteorder='little', signed=True)
    print(f"Received data: {data}")
    print(f"axis: {axis}, value: {value}")
    return axis, value


def move_mouse(axis, value):
    if axis == 0:    # X-axis
        device.emit(uinput.REL_X, value)
    elif axis == 1:  # Y-axis
        device.emit(uinput.REL_Y, value)


try:
    # sync package
    while True:
        print('Waiting for sync package...')
        while True:
            data = ser.read(1)
            if data == b'\xff':
                break

        # Read 5 bytes from UART
        data = ser.read(4)
        if len(data) < 4:
            continue
        print(f"Received data: {data}")
        print(f"Type: {data[0]}")


        if data[0] == 0:
            print("Mouse data")
            axis, value = parse_data(data)
            move_mouse(axis, value)


        elif data[0] == 1:
            print("Keyboard data")
            key = chr(data[1])
            status = data[2]
            print(f"Key: {key}")
            print(f"Status: {status}")
            if key == 'w':
                    keyboard.emit(uinput.KEY_W, status)
            if key == 'a':
                    keyboard.emit(uinput.KEY_A, status)
            if key == 's':
                    keyboard.emit(uinput.KEY_S, status)
            if key == 'd':
                    keyboard.emit(uinput.KEY_D, status)
            elif key == 'g':
                    keyboard.emit(uinput.KEY_SPACE, status)
            elif key == 'b':
                    keyboard.emit(uinput.KEY_LEFTCTRL, status)
            elif key == 'y':
                    keyboard.emit(uinput.KEY_LEFTSHIFT, status)

        else:
             print(data)

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
