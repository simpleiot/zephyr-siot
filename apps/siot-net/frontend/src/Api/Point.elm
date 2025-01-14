module Api.Point exposing
    ( Point
    , Resp
    , dataTypeFloat
    , dataTypeInt
    , dataTypeJSON
    , dataTypeString
    , fetch
    , get
    , getBool
    , getNum
    , getText
    , post
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
    , updatePoints
    )

import Http
import Json.Decode as Decode
import Json.Decode.Pipeline exposing (optional, required)
import Json.Encode as Encode
import List.Extra
import Url.Builder


type alias Point =
    { time : String
    , typ : String
    , key : String
    , dataType : String
    , data : String
    }


dataTypeInt : String
dataTypeInt =
    "INT"


dataTypeFloat : String
dataTypeFloat =
    "FLT"


dataTypeString : String
dataTypeString =
    "STR"


dataTypeJSON : String
dataTypeJSON =
    "JSN"


type alias Resp =
    { error : String
    }


fetch :
    { onResponse : Result Http.Error (List Point) -> msg
    }
    -> Cmd msg
fetch options =
    Http.get
        { url = Url.Builder.absolute [ "v1", "points" ] []
        , expect = Http.expectJson options.onResponse listDecoder
        }


post :
    { points : List Point
    , onResponse : Result Http.Error Resp -> msg
    }
    -> Cmd msg
post options =
    Http.post
        { url = Url.Builder.absolute [ "v1", "points" ] []
        , body = Http.jsonBody <| encodeList options.points
        , expect = Http.expectJson options.onResponse decodeResp
        }


encode : Point -> Encode.Value
encode p =
    Encode.object
        [ ( "time", Encode.string <| p.time )
        , ( "type", Encode.string <| p.typ )
        , ( "key", Encode.string <| p.key )
        , ( "dataType", Encode.string <| p.dataType )
        , ( "data", Encode.string <| p.data )
        ]


encodeList : List Point -> Encode.Value
encodeList p =
    Encode.list encode p


decoder : Decode.Decoder Point
decoder =
    Decode.succeed Point
        |> optional "time" Decode.string ""
        |> required "type" Decode.string
        |> required "key" Decode.string
        |> required "dataType" Decode.string
        |> required "data" Decode.string


listDecoder : Decode.Decoder (List Point)
listDecoder =
    Decode.list decoder


decodeResp : Decode.Decoder Resp
decodeResp =
    Decode.succeed Resp
        |> optional "error" Decode.string ""


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


getBool : List Point -> String -> String -> Bool
getBool points typ key =
    getNum points typ key == 1


getText : List Point -> String -> String -> String
getText points typ key =
    case get points typ key of
        Just p ->
            p.data

        Nothing ->
            ""


updatePoint : List Point -> Point -> List Point
updatePoint points point =
    case
        List.Extra.findIndex
            (\p ->
                point.typ == p.typ && point.key == p.key
            )
            points
    of
        Just index ->
            List.Extra.setAt index point points

        Nothing ->
            point :: points


updatePoints : List Point -> List Point -> List Point
updatePoints points newPoints =
    List.foldr
        (\newPoint updatedPoints -> updatePoint updatedPoints newPoint)
        points
        newPoints


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
