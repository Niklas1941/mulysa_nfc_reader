# Mulysa NFC reader

Simple nfc reader that can be used to activate doors / machines by asking mulysa if the card that was read is ok or not.

# Getting started

* power up the device
* wait a bit and connect to the AP the device is serving
* configure your wifi credentials and mulysa api endpoint
* wait a bit more
* try your card
* check in mulysa for the incoming request and get your deviceid and cardid from there
* add the deviceid to access devices
* add the cardid to your user
* retry the card, mulysa should respond http 200 and the output pin is toggled

# NOTE!

The code contains the root ca cert for doing tls. It will not be valid after

```
// Let's Encrypt ISRG Root X1
// valid untill 6/4/35, 2:04:38 PM GMT+3
```

Todo: figure out if this can be avoided