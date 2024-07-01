# State Machine

The main loop of the system is the state machine. 

```mermaid
graph TD
INIT
WAITING_FOR_QUEUE[WAIT FOR QUEUE]
INIT --[if music is already playing on owntone]-->WAITING_FOR_QUEUE
WAITING_FOR_QUEUE --[check at internal]-->SPINNING
INIT --[queue is empty]-->SPINNING
SPINNING --[once tag is read]-->QUEUING
SPINNING --[timeout]--> TAG_ERROR
TAG_ERROR --> ERROR
QUEUING --[200 server success]--> QUEUED
QUEUING --[server error]--> QUEUE_ERROR
QUEUE_ERROR --> ERROR
QUEUED --[needle turned on]--> PLAYING
PLAYING --[needle is off]--> PAUSED
PAUSED --[needled is turned on]--> PLAYING
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