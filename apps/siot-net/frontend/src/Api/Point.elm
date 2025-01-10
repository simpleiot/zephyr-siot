module Api.Point exposing
    ( Point
    , fetchList
    , get
    , getNum
    , getText
    , typeAddress
    , typeBoard
    , typeBootCount
    , typeDescription
    , typeGateway
    , typeMetricSysCPUPercent
    , typeNetmask
    , typeStaticIP
    , typeTemperature
    , typeUptime
    )

import Http
import Json.Decode as Decode
import Json.Decode.Pipeline exposing (optional, required)
import List.Extra
import Url.Builder


type alias Point =
    { time : String
    , typ : String
    , key : String
    , dataType : String
    , data : String
    }


fetchList :
    { onResponse : Result Http.Error (List Point) -> msg
    }
    -> Cmd msg
fetchList options =
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


get : List Point -> String -> String -> Maybe Point
get points typ key =
    let
        keyS =
            if key == "" then
                "0"

            else
                key
    in
    List.Extra.find
        (\p ->
            typ == p.typ && keyS == p.key
        )
        points


getNum : List Point -> String -> String -> Float
getNum points typ key =
    case get points typ key of
        Just p ->
            String.toFloat p.data |> Maybe.withDefault 0

        Nothing ->
            0


getText : List Point -> String -> String -> String
getText points typ key =
    case get points typ key of
        Just p ->
            p.data

        Nothing ->
            ""


typeDescription : String
typeDescription =
    "description"


typeStaticIP : String
typeStaticIP =
    "staticIP"


typeAddress : String
typeAddress =
    "address"


typeNetmask : String
typeNetmask =
    "netmask"


typeGateway : String
typeGateway =
    "gateway"


typeMetricSysCPUPercent : String
typeMetricSysCPUPercent =
    "metricSysCPUPercent"


typeTemperature : String
typeTemperature =
    "temp"


typeBoard : String
typeBoard =
    "board"


typeBootCount : String
typeBootCount =
    "bootCount"


typeUptime : String
typeUptime =
    "uptime"
