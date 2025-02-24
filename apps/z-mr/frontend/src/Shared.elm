module Shared exposing
    ( Flags
    , Model
    , Msg(..)
    , decoder
    , init
    , subscriptions
    , update
    )

import Json.Decode as Decode exposing (Decoder)
import Ports
import Shared.Model


type alias Flags =
    { width : Int
    , height : Int
    }


decoder : Decoder Flags
decoder =
    Decode.map2 Flags
        (Decode.field "width" Decode.int)
        (Decode.field "height" Decode.int)


type alias Model =
    Shared.Model.Model


type Msg
    = WindowResized { width : Int, height : Int }


init : Flags -> ( Model, Cmd Msg )
init flags =
    ( { windowWidth = flags.width
      , windowHeight = flags.height
      }
    , Cmd.none
    )


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        WindowResized dimensions ->
            ( { model
                | windowWidth = dimensions.width
                , windowHeight = dimensions.height
              }
            , Cmd.none
            )


subscriptions : Model -> Sub Msg
subscriptions _ =
    Ports.onWindowResize WindowResized 