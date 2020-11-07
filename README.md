# FireLights 
###### A wireless mesh network for lights

This project is based on fruitymesh, a bluetooth low energy mesh based on direct connections. It is used as firmware for custom
light nodes. The implementation is work in progress and uses the branch **firelight**

More information on fruitymesh can be found in the [fruitymesh documentation](https://www.bluerange.io/docs/bluerange-firmware/index.html)

This implementation uses custom modules, custom board config and messages. The firmware also implements PWM controlling for dimming LEDs.

### Functionality
...

### Message types
<style> table:nth-of-type(1) {  display:table;  width:100%;}</style>
<style> table:nth-of-type(2) {  display:table;  width:100%;}</style>
<style> table:nth-of-type(3) {  display:table;  width:100%;}</style>
<style> table:nth-of-type(4) {  display:table;  width:100%;}</style>
<style> table:nth-of-type(5) {  display:table;  width:100%;}</style>
<style> table:nth-of-type(6) {  display:table;  width:100%;}</style>


| Message | Payload size | Action trigger | Action response |
|----------|:-------------|:------:|:------:|
| PingLightMessage |  2 Bytes  | X | |
| ManualLightLevel |  2 Bytes  |  X | |
| GetDeviceInfo |  4 Bytes  |  X | |
| BasicDeviceInfo |  7 Bytes  |  | X |
| PartnerLights |  7 Bytes  |  X |  X |

#### PingLightMessage

Pings a light node that will light up for a short time. This is helpfull to find the correct light.

| Size | Structure | Description |
|:-------------:|:------|:------|
| **u16** |  timeoutInMs  | ping time out, default is 1000  |

#### ManualLightLevel

Set the light level of a light. The value is given in percent (0-100). When the light level was set by this message, the internal state machine will switch from automatic mode to manual mode and will stay there.

| Size | Structure | Description |
|:-------------:|:------|:------|
| **u16** |  level  | light level value in percent  |


#### GetDeviceInfo

This message is used as generic request message to gather device information. The *deviceInfoType* ist descirbed below the. Depending on the *deviceInfoType* the device will respond  different. 

| Size | Structure | Description |
|:-------------:|:------|:------|
| **u16** |  nodeId  | ID of the requested node |
| **u16** |  deviceInfoType  | specifies the requested data  |


| deviceInfoType  | Description |
|:-------------|:------|
| 0 - GET_BASIC_INFO  | Will respond with a **BasicDeviceInfo** message |
| 1 - GET_PARTNER_IDS  | Will respond with a **PartnerLights**  message |


#### BasicDeviceInfo

| Size | Structure | Description |
|:-------------:|:------|:------|
| **u16** |  nodeId  | ID of the requested node  |
| **u16** |  vsolar  | Solar voltage in mV (3200 = 3,2V)  |
| **u16** |  vbat  | Battery voltage in mV (4200 = 4,2V)  |
| **u8** |  pirState  | 1 if pir is active, otherwise 0   |

#### PartnerLights

| Size | Structure | Description |
|:-------------:|:------|:------|
| **u16** |  nodeId  | ID of the requested node - only need to be set when requesting data |
| **u16** |  previousLightId  | ID of the previous light node  |
| **u16** |  followingLightId  | ID of the following light node  |


Direction -->
(previousLightId) <--> (this node) <--> (followingLightId) 



