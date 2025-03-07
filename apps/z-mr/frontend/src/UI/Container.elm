module UI.Container exposing
    ( card
    , contentCard
    )

import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import UI.Device as Device exposing (Device)
import UI.Style as Style



-- Basic card container


card : Device -> List (Attribute msg) -> List (Element msg) -> Element msg
card device additionalAttributes content =
    column
        ([ spacing (Device.responsiveSpacing device 16)
         , padding (Device.responsiveSpacing device 24)
         , width fill
         , Background.color Style.colors.white
         , Border.rounded 12
         , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
         ]
            ++ additionalAttributes
        )
        content



-- Content card with title


contentCard : Device -> String -> List (Element msg) -> Element msg
contentCard device title content =
    card device
        []
        (paragraph
            [ Font.size (Device.responsiveFontSize device 24)
            , Font.semiBold
            , Font.color Style.colors.jet
            , paddingEach { top = 0, right = 0, bottom = Device.responsiveSpacing device 16, left = 0 }
            ]
            [ text title ]
            :: content
        )
