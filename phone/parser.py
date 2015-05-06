#qpy:console
BT_DEVICE_ID = '20:14:04:18:25:68'
auth = ('', '')


def read_bytes( number):
	start_time = time.time()
	bytes_rec = bytearray()
	while (len(bytes_rec) < number):
		if ((time.time() - start_time) > .5):
			print ("Timeout!")
			return bytearray()
		if(droid.bluetoothReadReady(connID).result == True):
			#only read how many bytes we need (number-len(bytes_rec))
			bytes_rec.extend(b64decode(droid.bluetoothReadBinary(number-len(bytes_rec),connID).result.encode()))
	#dump buffer if too much data
	if(droid.bluetoothReadReady(connID).result == True):
		print ("Too much data! Dumping Buffer")
		droid.bluetoothReadBinary(256,connID)
		return bytearray()
	return bytes_rec


def btconfirmation():
	bytes_rec = read_bytes(3)
	#check for incorrect reply
	if (bytes_rec != bytearray(b'\x00\xFF\x00')):
		print ("Bad Reply!")
		return False	
	return True
	
#max limits for colours
def brighten_color(color):
	if color == colour.Color("black"):
		raise Exception
	if (color.saturation < .5):
		color.saturation = .5
	if (color.luminance < .25):
		color.luminance = .25
	if (color.luminance > .75):
		color.luminance = .75
	return color
	
import os
os.system('clear')
print ("Loading...")
from colour import Color
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

blacklist = []

print ('Turning Bluetooth off...')
droid.toggleBluetoothState(False,False)  #turn BT off to kill old connections
time.sleep(0.1)
print ('Turning Bluetooth on...')
while (droid.checkBluetoothState().result == False): #turn BT on
	droid.toggleBluetoothState(True,False)  
	time.sleep(0.1)

print ('Bluetooth is on.')

while True:
	#stay awake
	droid.wakeLockAcquirePartial()	
	
	#keep connection to suit warm
	while (len(droid.bluetoothActiveConnections().result) == 0):
		print ("Reconnecting to Suit..."	)
		#this is the SPP UUID
		ssp_uuid = '00001101-0000-1000-8000-00805F9B34FB'
		droid.bluetoothConnect(ssp_uuid, BT_DEVICE_ID)
		time.sleep(0.1)
		if len(droid.bluetoothActiveConnections().result) != 0:
			#get the connidd - being lazy and using the first one in the list
			#if I had more than one bluetooth connection at a time I Would need to find my connid
			connID = list(droid.bluetoothActiveConnections().result.keys())[0]
			print ("Connected to suit.")
			print ("ID: " + connID)
			
			#dump the buffer
			if(droid.bluetoothReadReady(connID).result == True):
				droid.bluetoothReadBinary(256,connID)

	cycle_time = time.time()
	
	#ask suit for data
	#pre calculated COBs packet of 0x00 with CRC of 0x01 signifies request for data
	droid.bluetoothWriteBinary(b64encode(bytearray([0x00,0x01,0x01,0x01,0x00])).decode(),connID)
	
	payload = {}
	
	bytes_rec = read_bytes(37)
	#check for incorrect reply
	if len(bytes_rec) > 0:
		if cobs.getcrc(bytes_rec[:-1]) == bytes_rec[-1]:
			#print ("Good CRC!")
			payload['color1'] = Color(rgb=(int(bytes_rec[0])/255, int(bytes_rec[1])/255, int(bytes_rec[2])/255)).hex_l 
			payload['color2'] = Color(rgb=(int(bytes_rec[3])/255, int(bytes_rec[4])/255, int(bytes_rec[5])/255)).hex_l 
			
			payload['glove0cpu'] = int(bytes_rec[6])
			payload['glove1cpu'] = int(bytes_rec[7])
			payload['disc0cpu'] = int(bytes_rec[8])
			payload['suit0cpu'] = int(bytes_rec[9])
			
			payload['glove0ppsin'] = int(bytes_rec[16])
			payload['glove1ppsin'] = int(bytes_rec[17])
			payload['gloveppsout'] = int(bytes_rec[18])
			payload['glovelost'] = int(bytes_rec[19])
			
			payload['disc0ppsin'] = int(bytes_rec[20])			
			payload['disc0ppsout'] = int(bytes_rec[21])
			payload['disc0lost'] = int(bytes_rec[22])
			
			payload['glove0yaw'] = format((18000- int((bytes_rec[23] << 8) | bytes_rec[24]))/100, '.2f')
			payload['glove0pitch'] = format((18000-int((bytes_rec[25] << 8) | bytes_rec[26]))/100, '.2f')
			payload['glove0roll'] = format((18000-int((bytes_rec[27] << 8) | bytes_rec[28]))/100, '.2f')
			
			payload['glove1yaw'] = format((18000-int((bytes_rec[29] << 8) | bytes_rec[30]))/100, '.2f')
			payload['glove1pitch'] = format((18000-int((bytes_rec[31] << 8) | bytes_rec[32]))/100, '.2f')
			payload['glove1roll'] = format((18000-int((bytes_rec[33] << 8) | bytes_rec[34]))/100, '.2f')
			
			loc_result = droid.getLastKnownLocation().result['passive'];
			
			if (loc_result['latitude'] == 0):
				payload['latitude'] =  44.8615
			else:
				payload['latitude'] = format(loc_result['latitude'], '.4f')
				
			if (loc_result['longitude'] == 0):
				payload['longitude'] = -93.3531;
			else:
				payload['longitude'] = format(loc_result['longitude'], '.4f')
				
			if (loc_result['accuracy'] == 0):
				payload['accuracy'] = 1;
			else:
				payload['accuracy'] = loc_result['accuracy']
			
	pprint.pprint(payload)
	
	#get data from suit and post to web
	new_web_color = False
	new_sms_color = False
	new_web_message = False
	new_sms_message = False
	
	response = ''
	
	if (len(payload) > 0):
		try:
			r = s.post('https://.com/add.php', data=payload ,auth=auth)
			response =  str( r.content, encoding=r.encoding ) 
			r.close()
		except:
			print ("Blame urllib3!")

	
	response = response.strip()
	if (len(response) == 0):
		#check for cellphone requests if no web requests
		input = droid.smsGetMessages(True)	
		if (len(input.result) != 0):
		
			#put banning code here

			response = input.result[0]['body']
		
		
	colors = []
	words = []
	#split into segments
	for item in re.findall(r"#\w+|\w+", body):
		try:#try to make it a color
			colors.append(brighten_color(colour.Color(item)))
		except:#if it fails, we have a word
			words.append(item)

	#dupe color if we got just one
	if (len(colors) == 1):
		colors.append(colors[0])
	
	if (len(colors) >= 2):
		color_array = bytearray()
		for data in (colors[0].rgb + colors[1].rgb):
			color_array.append(int(data*255))
			
		if (sum(color_array) > 0 and len(color_array) == 6):
			new_color = True
			
	#strip HTML color codes (3 and 6 character with leading #)
	response = re.compile( r'#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})\b').sub("", body)
	
	#collapse extra spaces between words
	response = ' '.join(response.split())
	
	if (len(response) > 0):
		new_message=True
		message_firsthalf = response[:70].ljust(70)
		message_secondhalf = response[70:160].ljust(90)

		
	#execute requests
	if (new_color):
		print ("Color Incoming")
		item_sent = False
		retries = 0
		while (item_sent == False and retries < 2):
			droid.bluetoothWriteBinary(b64encode(cobs.encode(color_array)).decode(),connID)
			item_sent = btconfirmation()
			retries = retries + 1

	if (new_message):
		print ("Message Incoming")
		item_sent = False
		retries = 0
		while (item_sent == False and retries < 2):
			message_firsthalf = bytearray(map(ord, message_firsthalf))
			droid.bluetoothWriteBinary(b64encode(cobs.encode(message_firsthalf)).decode(),connID)
			item_sent = btconfirmation()
			retries = retries + 1

		item_sent = False
		retries = 0
		while (item_sent == False and retries < 2):
			message_secondhalf = bytearray(map(ord, message_secondhalf))
			droid.bluetoothWriteBinary(b64encode(cobs.encode(message_secondhalf)).decode(),connID)
			item_sent = btconfirmation()
			retries = retries + 1
			
	#as much as i'd like to be able to sleep for negative time if we are going slow... its not going to work	
	extra_time = max(1 - (time.time() - cycle_time),0)
	time.sleep(extra_time)
	
