module UI.Page exposing
    ( Layout(..)
    , TextSize(..)
    , header
    , paragraph
    , text
    , view
    )

import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import UI.Device as Device exposing (Device)
import UI.Nav as Nav
import UI.Style as Style



-- Layout options for different page types


type Layout msg
    = Standard Nav.Route -- Custom layout function



-- Text size presets


type TextSize
    = Small -- 14px base
    | Body -- 16px base
    | Large -- 24px base
    | Header -- 32px base



-- Main page layout


view :
    { title : String
    , device : Device
    , layout : Layout msg
    , header : Element msg
    , content : List (Element msg)
    }
    ->
        { title : String
        , attributes : List (Attribute msg)
        , element : Element msg
        }
view config =
    { title = config.title
    , attributes = []
    , element =
        case config.layout of
            Standard route ->
                standardLayout config.device route config.header config.content
    }



-- Standard layout with header, nav, and content


standardLayout : Device -> Nav.Route -> Element msg -> List (Element msg) -> Element msg
standardLayout device currentRoute header_ content =
    column
        [ spacing (Device.responsiveSpacing device 32)
        , padding (Device.responsiveSpacing device 40)
        , width (fill |> maximum Device.breakpoints.maxContentWidth)
        , height fill
        , centerX
        , Background.color Style.colors.pale
        ]
        (header_ :: Nav.view currentRoute :: content)



-- Full-width layout without nav
-- Standard page header with logo and title


header : Device -> String -> Element msg
header device title =
    column
        [ spacing (Device.responsiveSpacing device 16)
        , padding (Device.responsiveSpacing device 24)
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ row
            [ width fill
            , spacing (Device.responsiveSpacing device 16)
            ]
            [ image
                [ width
                    (fill
                        |> maximum
                            (if device.class == Device.Phone then
                                120

                             else
                                160
                            )
                    )
                , alignLeft
                ]
                { src = "https://zonit.com/wp-content/uploads/2023/10/zonit-primary-rgb-300.png"
                , description = "Z-MR"
                }
            , paragraph device
                Header
                [ Font.size (textSizeToPixels device Header)
                , Font.bold
                , Font.color Style.colors.jet
                , width fill
                ]
                [ Element.text title ]
            ]
        ]



-- Helper function to convert TextSize to actual pixel values based on device


textSizeToPixels : Device -> TextSize -> Int
textSizeToPixels device size =
    let
        baseSize =
            case size of
                Small ->
                    14

                Body ->
                    16

                Large ->
                    20

                Header ->
                    32
    in
    Device.responsiveFontSize device baseSize



-- Responsive text element


text : Device -> TextSize -> String -> Element msg
text device size content =
    el
        [ Font.size (textSizeToPixels device size) ]
        (Element.text content)



-- Responsive paragraph element


paragraph : Device -> TextSize -> List (Attribute msg) -> List (Element msg) -> Element msg
paragraph device size attrs content =
    Element.paragraph
        (Font.size (textSizeToPixels device size) :: attrs)
        content
