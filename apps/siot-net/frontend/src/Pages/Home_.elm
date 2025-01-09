module Pages.Home_ exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Font as Font
import Http
import Page exposing (Page)
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
    { points : List Point.Point
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model []
    , Effect.batch <|
        [ Effect.sendCmd <| Point.get { onResponse = ApiRespPointList }
        , Effect.sendCmd <| Task.perform Tick Time.now
        ]
    )


type Msg
    = NoOp
    | Tick Time.Posix
    | ApiRespPointList (Result Http.Error (List Point.Point))


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
        NoOp ->
            ( model
            , Effect.none
            )

        Tick _ ->
            ( model
            , Effect.sendCmd <| Point.get { onResponse = ApiRespPointList }
            )

        ApiRespPointList (Ok points) ->
            ( model
            , Effect.none
            )

        ApiRespPointList (Err httpError) ->
            ( model
            , Effect.none
            )



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions _ =
    Time.every 3000 Tick



-- VIEW


view : Model -> View Msg
view _ =
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
            , column [ spacing 10, padding 20 ]
                [ text "• Board: "
                , text "• Boot count: "
                , text "• CPU Usage: "
                , text "• Temperature: "
                ]
            , h1 "Settings"
            , h1 "Devices"
            ]
    }


h1 : String -> Element Msg
h1 txt =
    el [ Font.size 32, Font.bold ] <| text txt
