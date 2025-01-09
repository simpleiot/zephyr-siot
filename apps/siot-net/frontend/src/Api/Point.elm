module Api.Point exposing (Point, get)

import Http
import Json.Decode as Decode
import Json.Decode.Pipeline exposing (optional, required)
import Url.Builder


type alias Point =
    { time : String
    , typ : String
    , key : String
    , dataType : String
    , data : String
    }


get :
    { onResponse : Result Http.Error (List Point) -> msg
    }
    -> Cmd msg
get options =
    Http.get
        { url = Url.Builder.absolute [ "v1", "points" ] []
        , expect = Http.expectJson options.onResponse listDecoder
        }


listDecoder : Decode.Decoder (List Point)
listDecoder =
    Decode.list decoder


decoder : Decode.Decoder Point
decoder =
    Decode.succeed Point
        |> optional "time" Decode.string ""
        |> required "type" Decode.string
        |> required "key" Decode.string
        |> required "dataType" Decode.string
        |> required "data" Decode.string
