module Shared exposing
    ( Flags
    , Model
    , Msg
    , decoder
    , init
    , subscriptions
    , update
    )

import Effect exposing (Effect)
import Json.Decode
import Ports
import Route exposing (Route)
import Shared.Model
import Shared.Msg exposing (Msg(..))


type alias Flags =
    { width : Int
    , height : Int
    }


decoder : Json.Decode.Decoder Flags
decoder =
    Json.Decode.map2 Flags
        (Json.Decode.field "width" Json.Decode.int)
        (Json.Decode.field "height" Json.Decode.int)


type alias Model =
    Shared.Model.Model


init : Result Json.Decode.Error Flags -> Route () -> ( Model, Effect Msg )
init flagsResult route =
    case flagsResult of
        Ok flags ->
            ( { windowWidth = flags.width
              , windowHeight = flags.height
              }
            , Effect.none
            )
        
        Err _ ->
            -- Fallback to default dimensions if decoding fails
            ( { windowWidth = 1024
              , windowHeight = 768
              }
            , Effect.none
            )


update : Route () -> Msg -> Model -> ( Model, Effect Msg )
update route msg model =
    case msg of
        WindowResized dimensions ->
            ( { model
                | windowWidth = dimensions.width
                , windowHeight = dimensions.height
              }
            , Effect.none
            )


subscriptions : Route () -> Model -> Sub Msg
subscriptions _ model =
    Ports.onWindowResize WindowResized 