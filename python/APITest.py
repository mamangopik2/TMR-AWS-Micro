#create multiple tags

import requests
import json
import random
from datetime import datetime, timezone, timedelta
import time

def get_current_timestamp(delta_seconds: int = 0) -> str:
    # Get the current time in UTC
    current_time_utc = datetime.now(timezone.utc)

    # Adjust the time by the specified delta
    adjusted_time = current_time_utc + timedelta(seconds=delta_seconds)

    # Format the timestamp
    formatted_timestamp = adjusted_time.strftime('%Y-%m-%dT%H:%M:%SZ')

    return formatted_timestamp

headers = {
    "accept": "application/json",
    "x-ni-api-key": "FHBh0DgTFsPJ8g3Hn6df7nkFLi2oMV5lEMd8_qzLKd",
    "Content-Type": "application/json"
}
workspace = "5f9b3866-479f-4f5f-a08c-3d2faaf904fc"
url = "https://tmr-instrumentweb.com/nitag/v2/update-current-values"
local_timezone = "+07:00" 


print(url)

while(1):
  data = [
    {
        "path": "test-py",
        "properties": {
            "nitagRetention": "DURATION",
            "nitagHistoryTTLDays": "123",
            "nitagMaxHistoryCount": "50000"
        },
        "updates": [
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:30:30{local_timezone}"
            },
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:31:30{local_timezone}"
            }
            ,
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:32:30{local_timezone}"
            },
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:33:30{local_timezone}"
            }
        ],
        "workspace": workspace
    },
     {
        "path": "test-py2",
        "properties": {
            "nitagRetention": "DURATION",
            "nitagHistoryTTLDays": "123",
            "nitagMaxHistoryCount": "50000"
        },
        "updates": [
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:30:30{local_timezone}"
            },
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:31:30{local_timezone}"
            }
            ,
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:32:30{local_timezone}"
            },
            {
                "value": {
                  "type": "DOUBLE",
                  "value": random.randint(10,30)
                },
                "timestamp": f"2025-09-24T13:33:30{local_timezone}"
            }
        ],
        "workspace": workspace
    }
]
  data = [
            [
                {
                "path": "TMR_Weather_Station.Manggarai_Barat.1.TMP.AWS",
                "properties": {
                    "nitagRetention": "DURATION",
                    "nitagHistoryTTLDays": "360",
                    "nitagMaxHistoryCount": "50000"
                },
                "updates": [
                    {
                    "value": {
                        "type": "DOUBLE",
                        "value": "0.0000"
                    },
                    "timestamp": "2025-09-15T00:00:31+08:00"
                    }
                ],
                "workspace": "5f9b3866-479f-4f5f-a08c-3d2faaf904fc"
                }
            ],
            [
                {
                "path": "TMR_Weather_Station.Manggarai_Barat.1.HUMI.AWS",
                "properties": {
                    "nitagRetention": "DURATION",
                    "nitagHistoryTTLDays": "360",
                    "nitagMaxHistoryCount": "50000"
                },
                "updates": [
                    {
                    "value": {
                        "type": "DOUBLE",
                        "value": "0.0000"
                    },
                    "timestamp": "2025-09-15T00:00:31+08:00"
                    }
                ],
                "workspace": "5f9b3866-479f-4f5f-a08c-3d2faaf904fc"
                }
            ]
            ]
  print(data,"\r")
  response = requests.post(url, headers=headers, data=json.dumps(data[0]))
  

  # Attempt to parse the response as JSON
  try:
      print(response)
      response_json = response.json()
      print("Response JSON:", response_json)
  except requests.JSONDecodeError:
      print("Failed to decode JSON from response.")
  time.sleep(10)