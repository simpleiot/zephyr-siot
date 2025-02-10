module UI.Nav exposing (Route(..), view)

import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import UI.Style as Style


type Route
    = Live
    | Settings
    | Events


view : Route -> Element msg
view currentRoute =
    row
        [ spacing 4
        , padding 16
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ navLink currentRoute Live "Live View" "/live"
        , navLink currentRoute Settings "Settings" "/settings"
        , navLink currentRoute Events "Events" "/events"
        ]


navLink : Route -> Route -> String -> String -> Element msg
navLink currentRoute route label url =
    link
        [ padding 12
        , Border.rounded 8
        , if currentRoute == route then
            Background.color Style.colors.pale
          else
            Background.color Style.colors.white
        , if currentRoute == route then
            Font.color Style.colors.blue
          else
            Font.color Style.colors.jet
        , mouseOver
            [ Background.color Style.colors.pale
            , Font.color Style.colors.blue
            ]
        ]
        { url = url
        , label = text label
        } 