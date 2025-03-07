module UI.Style exposing
    ( button
    , colors
    , error
    , h2
    , link
    )

import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Html.Attributes as Attr



-- COLORS


colors :
    { primary : Color
    , secondary : Color
    , success : Color
    , danger : Color
    , warning : Color
    , info : Color
    , light : Color
    , dark : Color
    , white : Color
    , pale : Color
    , gray : Color
    , jet : Color
    , ledred : Color
    , ledgreen : Color
    , ledblue : Color
    , ltgray : Color
    , none : Color
    }
colors =
    { primary = rgb255 0 123 255
    , secondary = rgb255 108 117 125
    , success = rgb255 40 167 69
    , danger = rgb255 220 53 69
    , warning = rgb255 255 193 7
    , info = rgb255 23 162 184
    , light = rgb255 248 249 250
    , dark = rgb255 52 58 64
    , white = rgb255 255 255 255
    , pale = rgb255 245 245 245
    , gray = rgb255 108 117 125
    , jet = rgb255 50 50 50
    , ledred = rgb255 255 0 0
    , ledgreen = rgb255 0 255 0
    , ledblue = rgb255 0 0 255
    , ltgray = rgb255 211 211 211
    , none = rgba 0 0 0 0
    }


fonts : { sans : List Font.Font }
fonts =
    { sans =
        [ Font.external
            { name = "IBM Plex Sans"
            , url = "https://fonts.googleapis.com/css?family=IBM+Plex+Sans:400,400i,600,600i&display=swap"
            }
        , Font.serif
        ]
    }


link : List (Attribute msg)
link =
    [ Font.underline
    , Font.color colors.primary
    , transition
        { property = "opacity"
        , duration = 150
        }
    , mouseOver
        [ alpha 0.6
        ]
    ]


button : Color -> List (Attribute msg)
button color =
    [ paddingXY 16 8
    , Font.size 14
    , Border.color color
    , Font.color color
    , Background.color colors.white
    , Border.width 2
    , Border.rounded 4
    , pointer
    , transition
        { property = "all"
        , duration = 150
        }
    , mouseOver
        [ Font.color colors.white
        , Background.color color
        ]
    ]


error : List (Attribute msg)
error =
    [ paddingXY 16 8
    , Font.size 14
    , Font.color colors.danger
    , Font.bold
    , Background.color colors.pale
    , Border.width 2
    , Border.rounded 4
    , width fill
    ]


h2 : List (Attribute msg)
h2 =
    [ Font.family fonts.sans
    , Font.semiBold
    , Font.size 24
    ]


transition :
    { property : String
    , duration : Int
    }
    -> Attribute msg
transition { property, duration } =
    Element.htmlAttribute
        (Attr.style
            "transition"
            (property ++ " " ++ String.fromInt duration ++ "ms ease-in-out")
        )
