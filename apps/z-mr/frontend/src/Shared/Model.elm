module Shared.Model exposing (..)


type alias Model =
    { windowWidth : Int
    , windowHeight : Int
    }


init : Model
init =
    { windowWidth = 1024 -- Default desktop width
    , windowHeight = 768 -- Default desktop height
    }
