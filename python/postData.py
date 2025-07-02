#update multiple tags

import requests
import json
import random
from datetime import datetime, timezone,timedelta
import time

def generate_data(val, time,workspace,path,length):
  buffer = {
  "tags": [
  ],
  "merge": "false"
  }

  for i in range(length):
    buffer['tags'].append({
        "type": "DOUBLE",
        "value": val+i,
        "timestamp": time,
        "path": path+str(i),
        "workspace": workspace
      })

  return buffer



def get_current_timestamp(delta_seconds: int = 0) -> str:
    # Get the current time in UTC
    current_time_utc = datetime.now(timezone.utc)

    # Adjust the time by the specified delta
    adjusted_time = current_time_utc + timedelta(seconds=delta_seconds)

    # Format the timestamp
    formatted_timestamp = adjusted_time.strftime('%Y-%m-%dT%H:%M:%SZ')

    return formatted_timestamp

url = "https://tmr-instrumentweb.com/nitag/v2/update-current-values"
headers = {
    "accept": "application/json",
    "x-ni-api-key": "jWi0MYShG4KSOwaaQq5kbnGTrEGFCuyRrsPg2sZSvL",
    "Content-Type": "application/json"
}
data = [
  {
    "path": "demo.sommer.level",
    "updates": [
      {
        "value": {
          "type": "DOUBLE",
          "value": random.randint(10,30)
        }
      }
    ],
    "workspace": "828de439-1b9b-4dee-8dd4-838d92688668"
  },
  {
    "path": "demo.sommer.flow",
    "updates": [
      {
        "value": {
          "type": "DOUBLE",
          "value": random.randint(10,30)
        }
      }
    ],
    "workspace": "828de439-1b9b-4dee-8dd4-838d92688668"
  }
  ,
  {
    "path": "demo.sommer.rainfall",
    "updates": [
      {
        "value": {
          "type": "DOUBLE",
          "value": random.randint(10,30)
        }
      }
    ],
    "workspace": "828de439-1b9b-4dee-8dd4-838d92688668"
  }
  ,
  {
    "path": "demo.sommer.charge",
    "updates": [
      {
        "value": {
          "type": "DOUBLE",
          "value": random.randint(10,30)
        }
      }
    ],
    "workspace": "828de439-1b9b-4dee-8dd4-838d92688668"
  }
]

#data = [{"path":"Gn.Slamet.shelter15.AWS2.AWLR1","workspace":"828de439-1b9b-4dee-8dd4-838d92688668","updates":[{"value":{"type":"DOUBLE","value":0}}]},{"path":"Gn.Slamet.shelter15.AWS2.AWLR2","workspace":"828de439-1b9b-4dee-8dd4-838d92688668","updates":[{"value":{"type":"DOUBLE","value":0}}]}]




while(1):
  #print(data,"\r")
  response = requests.post(url, headers=headers, data=json.dumps(data))

  # Attempt to parse the response as JSON
  try:
      print(response)
      #response_json = response.json()
      #print("Response JSON:", response_json)
  except requests.JSONDecodeError:
      print("Failed to decode JSON from response.")
  time.sleep(10)