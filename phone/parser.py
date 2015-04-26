#qpy:console

DSCRC_TABLE = [
    0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65, 
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220, 
    35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98, 
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255, 
    70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7, 
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154, 
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36, 
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185, 
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205, 
    17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80, 
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238, 
    50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115, 
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139, 
    87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22, 
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168, 
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
]

BT_DEVICE_ID = '20:14:04:18:25:68'

import os
os.system('clear')
print ("Loading...")

import requests
import time
import pprint
import cobs
from base64 import b64encode
from base64 import b64decode

#from androidhelper import Android  #old way
import sl4a  #new way? seems to work the same

s = requests.Session()
#droid = Android() #old way
droid = sl4a.Android() #new way? seems to work the same

print ('Turning Bluetooth off...')
droid.toggleBluetoothState(False,False)  #turn BT off to kill old connections

print ('Turning Bluetooth on...')
while (droid.checkBluetoothState().result == False): #turn BT on
	droid.toggleBluetoothState(True,False)  
	time.sleep(1)

print ('Bluetooth is on.')
 
while True:
	#keep connection to suit warm
	while (len(droid.bluetoothActiveConnections().result) == 0):
		print ("Reconnecting to Suit..."	)
		#this is the SPP UUID
		ssp_uuid = '00001101-0000-1000-8000-00805F9B34FB'
		droid.bluetoothConnect(ssp_uuid, BT_DEVICE_ID)
		time.sleep(1)
		if len(droid.bluetoothActiveConnections().result) != 0:
			#get the connidd - being lazy and using the first one in the list
			#if I had more than one bluetooth connection at a time I Would need to find my connid
			connID = list(droid.bluetoothActiveConnections().result.keys())[0]
			print ("Connected to suit.")
			print ("connID: " + connID)
			

	#stay awake
	droid.wakeLockAcquirePartial()
	
	#get data from suit and post to web
	new_web_color = False
	new_sms_color = False
	new_web_message = False
	
	auth = ('username', 'password')
	payload = {'number1' : '1', 'number2' : '10'}
	
	response = ''
	try:
		#start_time = time.time()
		r = s.post('siteurl.com/add.php', data=payload ,auth=auth)
		#print("--- %s seconds ---" % (time.time() - start_time))
		response =  str( r.content, encoding=r.encoding ) 
		r.close()
	except:
		print ("BLAME URLLIB3")
	
	response = response.split('\n')
	
	if (len(response) > 0):
		
		last_response = response[0]
		response = response[0].split('\t')		
		pprint.pprint(response)
		if (len(response) > 2):
			color1r = int(response[2])
			color1g = int(response[3])
			color1b = int(response[4])
			color2r = int(response[5])
			color2g = int(response[6])
			color2b = int(response[7])
			
			if (color1r + color1g + color1b + color2r + color2g + color2b != 0):
				new_web_color = True
				print ("Web Colour Incoming!")
				
			if (len(response[1].strip()) > 1):
				new_web_message=True
				print ("Web message Incoming!")
				message_firsthalf = response[1][:70].ljust(70)
				message_secondhalf = response[1][70:160].ljust(90)
				print (message_firsthalf)
				print (message_secondhalf)
			

			

	
	#check for cellphone requests
	
	
	#check for web requests
	
	

	#execute requests


	if (new_web_color):		
		color_array = bytearray([color1r,color1g,color1b,color2r,color2g,color2b])

	if (new_web_color or new_sms_color):	
	
		crc = 0
		for i in color_array:
			crc = DSCRC_TABLE[crc ^ i]
			
		color_array.append(crc)
		droid.bluetoothWriteBinary(b64encode(cobs.encode(color_array)).decode(),connID)
		
	if (new_web_message):

		message_firsthalf = bytearray(map(ord, message_firsthalf))
		crc = 0
		for i in message_firsthalf:
			crc = DSCRC_TABLE[crc ^ i]
			
		message_firsthalf.append(crc)
		droid.bluetoothWriteBinary(b64encode(cobs.encode(message_firsthalf)).decode(),connID)
		
		time.sleep(1)
		
		message_secondhalf = bytearray(map(ord, message_secondhalf))
		crc = 0
		for i in message_secondhalf:
			crc = DSCRC_TABLE[crc ^ i]
			
		message_secondhalf.append(crc)
		droid.bluetoothWriteBinary(b64encode(cobs.encode(message_secondhalf)).decode(),connID)
	
	time.sleep(1)