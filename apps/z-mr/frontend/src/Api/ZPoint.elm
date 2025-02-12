module Api.ZPoint exposing
    ( typeAtsA
    , typeAtsB
    , typeFanMode
    , typeFanSetSpeed
    , typeFanSpeed
    , typeSNMPServer
    , typeSwitch
    , typeTestLEDs
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
