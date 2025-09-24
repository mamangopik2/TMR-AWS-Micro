import pandas as pd
import requests
import json
import random
import math
import time

# ====== Konfigurasi ======
url = "https://tmr-instrumentweb.com/nitag/v2/update-current-values"
api_key = "FHBh0DgTFsPJ8g3Hn6df7nkFLi2oMV5lEMd8_qzLKd"          # ganti dengan API key kamu
workspace_id = "5f9b3866-479f-4f5f-a08c-3d2faaf904fc"  # ganti dengan workspace ID kamu
csv_path = "E:/Users/user/Downloads/DATA_LOG_2025-09-24.csv"   # path file CSV
local_timezone = "+07:00"
# # ====== Load CSV ======

payload = []

logFile = open(csv_path,'r')
rawLogDatas = logFile.readlines()
logFile.close()

cnt = 0

tags = []
for line in rawLogDatas:
    line = line.replace("\n","")
    cols = []
    if(len(cols)<=0):
        cols = line.split(",")
    if(len(cols)<=0):
        cols = line.split(";")

    if(len(cols)>1):
        ts = cols[0]
        tag = cols[1]
        scaledVal = cols[2]
        unscaledVal = cols[3]
        engUnit = cols[4]
        rawUnit = cols[5]

        ts  = ts.replace('Z','')
        tags.append(tag)

tags = list(dict.fromkeys(tags)) #remove repetition from keys
# print(tags)
template = {
        "path": "",
        "workspace": workspace_id,
        "updates": []
    }


entries = {}

for tagName in tags:
    # print(tagName)
    for line in rawLogDatas:
        line = line.replace("\n","")
        cols = []
        if(len(cols)<=0):
            cols = line.split(",")
        if(len(cols)<=0):
            cols = line.split(";")

        if(len(cols)>1):
            ts = cols[0]
            tag = cols[1]
            scaledVal = cols[2]
            unscaledVal = cols[3]
            engUnit = cols[4]
            rawUnit = cols[5]

            ts  = ts.replace('Z','')

            if(tag==tagName):
                template["path"]=tag
                template["updates"].append(
                    {
                        "value": {
                        "type": "DOUBLE",
                        "value": scaledVal
                        },
                        "timestamp": f"{ts}{local_timezone}"
                    }
                )
    entries[tagName]=template
    template = {
        "path": "",
        "workspace": workspace_id,
        "updates": []
    }


# print(json.dumps(entries))
print(tags)


max_tags = 10
max_batch = max_tags
index=0
buffer = []

for entry in entries:
    datas = entries[entry]["updates"]  # bisa lebih dari 900
    repetition = math.floor(len(datas) / max_batch)
    remainer = len(datas) % max_batch
    index = 0  # reset index per entry

    print(type(datas))
    print(repetition)
    print(len(datas))
    print("last source index", index)

    for rep in range(repetition):
        print(f"batch:{rep} max={max_batch*rep} last_sourceindex:{index}")

        buffer = [] 
        for i in range(max_batch):
            buffer.append(datas[index])  # pakai index untuk ambil data
            index += 1
        
        # ====== Kirim ke API ======
        print(f"max packet{max_tags}, total tags in {entries[entry]["path"]} = {len(datas)}")
        print(f"sending batch:{rep}")
        headers = {
            "accept": "application/json",
            "x-ni-api-key": api_key,
            "Content-Type": "application/json"
        }

        packet = {
            "path": entries[entry]["path"],
            "workspace": workspace_id,
            "updates": buffer   # potong sesuai max_tags
        }
        endpoint = url.format(workspace=workspace_id)
        response = requests.post(endpoint, headers=headers, json=[packet])
        # print(json.dumps(packet))
        print("Status Code:", response.status_code)
        print("Response:", response.text)
        print("batch size:", len(buffer))
        time.sleep(1)

    # sisa data (kalau ada)
    if remainer > 0:
        buffer = datas[index:index+remainer]
        index += remainer
        print("last remainder batch size:", len(buffer))

    # ====== Kirim ke API ======
        print(f"max packet{max_tags}, total tags in {entries[entry]["path"]} = {len(datas)}")
        print(f"sending batch:{rep}")
        headers = {
            "accept": "application/json",
            "x-ni-api-key": api_key,
            "Content-Type": "application/json"
        }

        packet = {
            "path": entries[entry]["path"],
            "workspace": workspace_id,
            "updates": buffer   # potong sesuai max_tags
        }
        endpoint = url.format(workspace=workspace_id)
        response = requests.post(endpoint, headers=headers, json=[packet])
        # print(json.dumps(packet))
        print("Status Code:", response.status_code)
        print("Response:", response.text)
        print("batch size:", len(buffer))
        time.sleep(1)