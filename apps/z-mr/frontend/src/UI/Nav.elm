module UI.Nav exposing (Route(..), view)

import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import UI.Style as Style
import Debug


type Route
    = Live
    | Settings
    | Events


view : Route -> Element msg
view currentRoute =
    row
        [ spacing 8
        , padding 12
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        , centerX
        ]
        [ row
            [ centerX
            , spacing 8
            , width (fill |> maximum 600)
            ]
            [ navLink currentRoute Live "Live" "/live"
            , navLink currentRoute Settings "Settings" "/settings"
            , navLink currentRoute Events "Events" "/events"
            ]
        ]


navLink : Route -> Route -> String -> String -> Element msg
navLink currentRoute route label url =
    let
        isActive = currentRoute == route
        _ = Debug.log "Nav route comparison" 
            { current = Debug.toString currentRoute
            , target = Debug.toString route
            , isActive = isActive
            , label = label
            }
    in
    link
        ([ padding 12
         , Border.rounded 8
         , width (fillPortion 1)
         ]
            ++ (if isActive then
                    [ Background.color Style.colors.pale
                    , Font.color Style.colors.primary
                    ]

                else
                    [ Background.color Style.colors.white
                    , Font.color Style.colors.jet
                    ]
               )
            ++ [ mouseOver
                    [ Background.color Style.colors.pale
                    , Font.color Style.colors.primary
                    ]
               ]
        )
        { url = url
        , label = el [ centerX ] (text label)
        }
