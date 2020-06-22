#----------------------------------------------------------------------------------------------------------
#
#			Name: Eng. William da Rosa Frohlich
#
#			Project: Processing of BITalino data
#
#			Date: 2020.06.13
#
#----------------------------------------------------------------------------------------------------------
#----------			Libraries			----------

import math
import json
import socket
import datetime
import psycopg2
import numpy as np
import pandas as pd
import paho.mqtt.client as mqtt
from time import sleep
from biosppy import signals as bio_signals

#----------------------------------------------------------------------------------------------------------
#----------			Variables			----------

#----------			Variables - Socket			----------
# Create the UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

address =('',18002)
sock.bind(address)

#----------			Variables - Database			----------
DSN = 'dbname=bitalino user=postgres host=localhost port=18001'

#----------			Variables - MQTT			----------
ORG = "t5d7s9"
DEVICE_TYPE = "raspberry" 
TOKEN = "BITalino_Server"
DEVICE_ID = "b827eb101d5d"

server = ORG + ".messaging.internetofthings.ibmcloud.com";
event = "iot-2/evt/bitalino/fmt/json";
authMethod = "use-token-auth";
token = TOKEN;
clientId = "d:" + ORG + ":" + DEVICE_TYPE + ":" + DEVICE_ID;

#----------			Variable - Processing			----------
data_processing = pd.DataFrame(columns =['ECG','EDA','TIME'])

#----------------------------------------------------------------------------------------------------------
#----------			Functions			----------

#----------			Function - Save File With Raw Data			----------
def save_file(name, data_processing):
	name_file = ("Patient - " + name + ".csv")
	data_processing.to_csv(name_file, index=None, header=True, sep=';', encoding='utf-8')
	print("File saved.")
	
#----------			Function - Processing the Data			----------
def processing(data_processing):
	ecg_signal = bio_signals.ecg.ecg(signal=data_processing['ECG'], sampling_rate=100, show=False)
	eda_signal = bio_signals.eda.eda(signal=data_processing['EDA'], sampling_rate=100, show=False)
	ECG_average = ecg_signal["heart_rate"].mean()
	EDA_average = eda_signal["filtered"].mean()
	start_time = str(data_processing['TIME'][0:1].values)
	size = len(start_time)
	start_time = start_time[2:(size-2)]
	end = len(data_processing)
	final_time = str(data_processing['TIME'][(end-2):(end-1)].values)
	size = len(final_time)
	final_time = final_time[2:(size-2)]
	print("Data processed.")
	
	return ECG_average, EDA_average, start_time, final_time

#----------			Function - Processing to get ECG			----------
def heart_rate_process(name, data_processing):
	ecg_signal = bio_signals.ecg.ecg(signal=data_processing['ECG'], sampling_rate=100, show=False)
	size=len(ecg_signal["heart_rate"])
	x=0
	while size > x:
		heart_rate = float(ecg_signal["heart_rate"][x:x+1])
		heart_rate_ts = float(ecg_signal["heart_rate_ts"][x:x+1])
		register_database_ecg (name, heart_rate, heart_rate_ts)
		x = x + 1 

#----------			Function - Register in database - Raw data			----------
def register_database_raw_data (name, ecg, eda, time):
	connecting = psycopg2.connect(DSN)
	SQL = 'INSERT INTO raw_data (name, ecg, eda, time) VALUES (%s, %s, %s, %s)'
	cursorsql = connecting.cursor()
	cursorsql.execute(SQL, (name, ecg, eda, time))
	print("Registered raw data.")
	connecting.commit()
	cursorsql.close()

#----------			Function - Register in database - ECG			----------
def register_database_ecg (name, heart_rate, heart_rate_ts):	
	connecting = psycopg2.connect(DSN)
	SQL = 'INSERT INTO ecg (name, heart_rate, heart_rate_ts) VALUES (%s, %s, %s)'
	cursorsql = connecting.cursor()
	cursorsql.execute(SQL, (name, heart_rate, heart_rate_ts))
	print("Registered ECG data.")
	connecting.commit()
	cursorsql.close()
	
#----------			Function - Register in database - Processed Data			----------
def register_database_processed (name, ecg, eda, start_time, final_time):	
	connecting = psycopg2.connect(DSN)
	SQL = 'INSERT INTO processed (name, ecg, eda, start_time, final_time) VALUES (%s, %s, %s, %s, %s)'
	cursorsql = connecting.cursor()
	cursorsql.execute(SQL, (name, ecg, eda, start_time, final_time))
	print("Registered processed data.")
	connecting.commit()
	cursorsql.close()
	
#----------			Function - Connect to Cloud Server			----------
def ibm_watson (name, ecg, eda):
	mqttc = mqtt.Client(client_id=clientId)
	mqttc.username_pw_set(authMethod, token)
	mqttc.connect(server, 1883, 60)
	payload = {'Name':name,'ECG':ecg,'EDA':eda}
	mqttc.publish(event, json.dumps(payload))
	print ("Published in the Cloud.")
	sleep(5);
	mqttc.loop()
	mqttc.disconnect()

#----------------------------------------------------------------------------------------------------------
#----------			Main Function			----------
while True:
	try:
		packet, addr = sock.recvfrom(1024)
		packet = (str(packet))
		size = len(packet)
		control = int(packet[2:3])
		data = packet[3:(size-1)]
			
		if (control == 1):
			name = data
		elif (control == 2):
			ECG = int(data)
		elif (control == 3):
			EDA = int(data)
		elif (control == 4):
			time = data
		else:
			if (data == "STOP"):
				save_file(name, data_processing)
				heart_rate_process(name, data_processing)
				ECG_average, EDA_average, start_time, final_time = processing(data_processing)
				ibm_watson(name, ECG_average, EDA_average)
				register_database_processed(name, ECG_average, EDA_average, start_time, final_time)
				data_processing = pd.DataFrame(columns =['ECG','EDA','TIME'])
			else:
				register_database_raw_data(name, ECG, EDA, time)
				new_df = pd.DataFrame({'ECG' : [ECG], 'EDA' : [EDA], 'TIME' : [time]})
				data_processing = data_processing.append(new_df)		
	
	except KeyboardInterrupt:
		break
#----------------------------------------------------------------------------------------------------------