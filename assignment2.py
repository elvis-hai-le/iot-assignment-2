import serial
import pymysql

device = '/dev/ttyS0'

arduino = serial.Serial(device,115200)
while True:
    data = arduino.readline().decode('utf-8').strip()
    if (float(data) > 28.00):
        arduino.write(bytes("AC COOLING",'utf-8'))
        print("AC cooling on")
    print(data)

    dbConn = pymysql.connect("localhost","admin","","temperature_db") or die("Could not connect to database")

    print (dbConn)

    with dbConn:
        cursor = dbConn.cursor();
        cursor.execute("INSERT INTO tempLog (Temperature) VALUES (%s)" % (data))
        cursor.execute("INSERT INTO tempLog (date_time)")
        dbConn.commit()
        cursor.close()