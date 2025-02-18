module Api.ZPoint exposing
    ( typeAtsA
    , typeAtsB
    , typeFanMode
    , typeFanSetSpeed
    , typeFanSpeed
    , typeFanStatus
    , typeSNMPServer
    , typeSwitch
    , typeTestLEDs
    , valueOff
    , valuePwm
    , valueTach
    , valueTemp
    )

-- These defines should match zpoint.h


typeAtsA : String
typeAtsA =
    "atsA"


typeAtsB : String
typeAtsB =
    "atsB"


typeSNMPServer : String
typeSNMPServer =
    "snmpServer"


typeSwitch : String
typeSwitch =
    "switch"


typeTestLEDs : String
typeTestLEDs =
    "testLEDs"


typeFanMode : String
typeFanMode =
    "fanMode"


typeFanSetSpeed : String
typeFanSetSpeed =
    "fanSetSpeed"


typeFanSpeed : String
typeFanSpeed =
    "fanSpeed"


valueOff : String
valueOff =
    "off"


valueTemp : String
valueTemp =
    "temp"


valueTach : String
valueTach =
    "tach"


valuePwm : String
valuePwm =
    "pwm"


typeFanStatus : String
typeFanStatus =
    "fanStatus"
