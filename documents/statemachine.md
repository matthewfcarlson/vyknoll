# State Machine

The main loop of the system is the state machine. 

```mermaid
graph TD
WAITING_FOR_WIFI[WAIT FOR WIFI]
INIT--[wifi creds in memory]-->WAITING_FOR_WIFI
INIT --[no creds]-->WIFI_SETUP
WIFI_SETUP --> REBOOT
WAITING_FOR_WIFI --[timeout] --> WIFI_SETUP
WAITING_FOR_WIFI --> CHECK_OTA
WAITING_FOR_QUEUE[WAIT FOR QUEUE]
CHECK_OTA --[if OTA is available]-->OTA
CHECK_OTA --[no OTA] --> CHECK_POWERSWITCH
CHECK_POWERSWITCH --[power on]-->CHECK_OWNTONE
CHECK_POWERSWITCH --[power off]-->OFF
OFF --[power on]-->CHECK_OWNTONE
OTA --[error]-->OTA_ERROR
OTA_ERROR --> ERROR
CHECK_OWNTONE --[owntone isn't available]--> WIFI_SETUP
OTA --[success]-->REBOOT
ERROR --[button press]-->REBOOT
CHECK_OWNTONE --[if music is already playing on owntone]-->WAITING_FOR_QUEUE
WAITING_FOR_QUEUE --[check at internal]-->SPINNING
CHECK_OWNTONE --[queue is empty]-->SPINNING
SPINNING --[once tag is read]-->QUEUING
SPINNING --[timeout]--> TAG_ERROR
TAG_ERROR --> ERROR
QUEUING --[200 server success]--> QUEUED
QUEUING --[server error]--> QUEUE_ERROR
QUEUE_ERROR --> ERROR
QUEUED --[needle turned on]--> PLAYING
QUEUED --[power off]-->DRAINING
QUEUED --[queue is emptied]-->QUEUE_ERROR
DRAINING --[queue empty or timeout] --> OFF
PLAYING --[needle is off]--> PAUSED
PAUSED --[needled is turned on]--> PLAYING
PAUSED --[power switch off] --> DRAINING
PAUSED --[queue is cleared]--> FINISHED
PLAYING --[queue is cleared]--> FINISHED
FINISHED --[needle is off]--> DONE
DONE --[needle is on]--> SPINNING
```

- INIT = the first state that we start in
- WAITING_FOR_QUEUE = the queue is not empty, let the queue drain
- SPINNING = the record starts spinning to try and read it
- QUEUEING = we have read a tag but we are adding it to the queue
- QUEUED = server has queued the album
- PLAYING = server is playing
- PAUSED = server is paused
- FINISHED = server has emptied the queue