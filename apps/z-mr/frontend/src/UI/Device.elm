module UI.Device exposing
    ( Device
    , DeviceClass(..)
    , Orientation(..)
    , breakpoints
    , classifyDevice
    , responsiveFontSize
    , responsiveSpacing
    )

-- DEVICE CLASSIFICATION
type DeviceClass
    = Phone
    | Tablet
    | Desktop

type Orientation
    = Portrait
    | Landscape

type alias Device =
    { class : DeviceClass
    , orientation : Orientation
    , width : Int
    , height : Int
    }

-- BREAKPOINTS (in logical pixels)
breakpoints =
    { phone = 428  -- iPhone Pro Max logical width
    , tablet = 768  -- iPad Mini logical width
    , desktop = 1024  -- Standard desktop breakpoint
    , maxContentWidth = 1280  -- Maximum content width
    }

-- DEVICE CLASSIFICATION
classifyDevice : Int -> Int -> Device
classifyDevice width height =
    let
        aspectRatio =
            toFloat width / toFloat height
        
        orientation =
            if aspectRatio < 1 then
                Portrait
            else
                Landscape
                
        deviceClass =
            if width <= breakpoints.phone then
                Phone
            else if width <= breakpoints.tablet then
                Tablet
            else
                Desktop
    in
    { class = deviceClass
    , orientation = orientation
    , width = width
    , height = height
    }

-- RESPONSIVE UTILITIES
responsiveSpacing : Device -> Int -> Int
responsiveSpacing device base =
    case device.class of
        Phone ->
            base // 2
        
        Tablet ->
            if device.orientation == Portrait then
                base * 3 // 4
            else
                base
        
        Desktop ->
            base

responsiveFontSize : Device -> Int -> Int
responsiveFontSize device base =
    case device.class of
        Phone ->
            base * 4 // 5
        
        Tablet ->
            if device.orientation == Portrait then
                base * 9 // 10
            else
                base
        
        Desktop ->
            base 