module Pages.Home_ exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Border as Border
import Element.Font as Font
import Http
import Page exposing (Page)
import Round
import Route exposing (Route)
import Shared
import Task
import Time
import View exposing (View)


page : Shared.Model -> Route () -> Page Model Msg
page _ _ =
    Page.new
        { init = init
        , update = update
        , subscriptions = subscriptions
        , view = view
        }



-- INIT


type alias Model =
    { points : Api.Data (List Point.Point)
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading
    , Effect.batch <|
        [ Effect.sendCmd <| Point.fetchList { onResponse = ApiRespPointList }
        , Effect.sendCmd <| Task.perform Tick Time.now
        ]
    )


type Msg
    = NoOp
    | Tick Time.Posix
    | ApiRespPointList (Result Http.Error (List Point))


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
        NoOp ->
            ( model
            , Effect.none
            )

        Tick _ ->
            ( model
            , Effect.sendCmd <| Point.fetchList { onResponse = ApiRespPointList }
            )

        ApiRespPointList (Ok points) ->
            ( { model | points = Api.Success points }
            , Effect.none
            )

        ApiRespPointList (Err httpError) ->
            ( { model | points = Api.Failure httpError }
            , Effect.none
            )



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions _ =
    Time.every 3000 Tick



-- VIEW


view : Model -> View Msg
view model =
    { title = "sIoT"
    , attributes = []
    , element =
        column [ spacing 20, padding 60, width (fill |> maximum 1280), height fill, centerX ]
            [ row [ spacing 32 ]
                [ image [ width (px 140) ]
                    { src = "https://docs.simpleiot.org/docs/images/siot-logo.png?raw=true"
                    , description = "SIOT logo"
                    }
                ]
            , h1 "Status"
            , status model
            , h1 "Settings"
            , h1 "Devices"
            ]
    }


h1 : String -> Element Msg
h1 txt =
    el [ Font.size 32, Font.bold ] <| text txt


status : Model -> Element Msg
status model =
    case model.points of
        Api.Loading ->
            text "Loading ..."

        Api.Success points ->
            statusTable points

        Api.Failure httpError ->
            text <| "Lost connection: " ++ Api.toUserFriendlyMessage httpError


statusTable : List Point -> Element Msg
statusTable points =
    let
        cell =
            el [ paddingXY 15 5, Border.width 1 ]

        data =
            [ { name = "Board", value = Point.getText points Point.typeBoard "0" }
            , { name = "Boot count", value = Point.getText points Point.typeBootCount "0" }
            , { name = "CPU Usage", value = Round.round 2 (Point.getNum points Point.typeMetricSysCPUPercent "0") ++ "%" }
            , { name = "Uptime", value = Point.getText points Point.typeUptime "0" ++ "s" }
            ]
    in
    table [ padding 0, Border.width 1, width shrink ]
        { data = data
        , columns =
            [ { header = text ""
              , width = shrink
              , view = \m -> cell <| text m.name
              }
            , { header = text ""
              , width = shrink
              , view = \m -> cell <| text m.value
              }
            ]
        }
