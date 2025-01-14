module Pages.Home_ exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Http
import Page exposing (Page)
import Round
import Route exposing (Route)
import Shared
import Task
import Time
import UI.Form as Form
import UI.Style as Style
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
    { points : Api.Data (List Point)
    , pointMods : List Point
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading []
    , Effect.batch <|
        [ Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
        , Effect.sendCmd <| Task.perform Tick Time.now
        ]
    )


type Msg
    = NoOp
    | Tick Time.Posix
    | ApiRespPointList (Result Http.Error (List Point))
    | ApiRespPointPost (Result Http.Error Point.Resp)
    | EditPoint (List Point)
    | ApiPostPoints (List Point)
    | DiscardEdits


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
        NoOp ->
            ( model
            , Effect.none
            )

        Tick _ ->
            ( model
            , Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
            )

        ApiRespPointList (Ok points) ->
            ( { model | points = Api.Success points }
            , Effect.none
            )

        ApiRespPointList (Err httpError) ->
            ( { model | points = Api.Failure httpError }
            , Effect.none
            )

        ApiRespPointPost (Ok _) ->
            ( model
            , Effect.none
            )

        ApiRespPointPost (Err httpError) ->
            ( { model | points = Api.Failure httpError }
            , Effect.none
            )

        EditPoint points ->
            ( { model | pointMods = Point.updatePoints model.pointMods points }
            , Effect.none
            )

        ApiPostPoints points ->
            -- optimistically update points?
            ( { model | points = Api.Success points, pointMods = [] }
            , Effect.sendCmd <| Point.post { points = model.pointMods, onResponse = ApiRespPointPost }
            )

        DiscardEdits ->
            ( { model | pointMods = [] }
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
            , deviceContent model
            , h1 "Devices"
            ]
    }


h1 : String -> Element Msg
h1 txt =
    el [ Font.size 32, Font.bold ] <| text txt


deviceContent : Model -> Element Msg
deviceContent model =
    case model.points of
        Api.Loading ->
            text "Loading ..."

        Api.Success points ->
            let
                pointsMerge =
                    Point.updatePoints points model.pointMods
            in
            column [ spacing 20 ]
                [ h1 "Status"
                , statusTable pointsMerge
                , h1 "Settings"
                , settings pointsMerge (List.length model.pointMods > 0)
                ]

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


settings : List Point -> Bool -> Element Msg
settings points edit =
    column [ spacing 20, Form.onEnterEsc (ApiPostPoints points) DiscardEdits ]
        [ inputText points "0" Point.typeDescription "Description" "desc"
        , inputCheckbox points "0" Point.typeStaticIP "Static IP"
        , viewIf edit <|
            Form.buttonRow <|
                [ Form.button
                    { label = "save"
                    , color = Style.colors.blue
                    , onPress = ApiPostPoints points
                    }
                , Form.button
                    { label = "discard"
                    , color = Style.colors.gray
                    , onPress = DiscardEdits
                    }
                ]
        ]


inputText : List Point -> String -> String -> String -> String -> Element Msg
inputText pts key typ lbl placeholder =
    let
        textRaw =
            Point.getText pts typ key

        labelWidth =
            120
    in
    Input.text
        []
        { onChange =
            \d ->
                EditPoint [ Point "" typ key Point.dataTypeString d ]
        , text =
            if textRaw == "123BLANK123" then
                ""

            else
                let
                    v =
                        Point.getNum pts typ key
                in
                if v /= 0 then
                    ""

                else
                    textRaw
        , placeholder = Just <| Input.placeholder [] <| text placeholder
        , label =
            if lbl == "" then
                Input.labelHidden ""

            else
                Input.labelLeft [ width (px labelWidth) ] <| el [ alignRight ] <| text <| lbl ++ ":"
        }


inputCheckbox : List Point -> String -> String -> String -> Element Msg
inputCheckbox pts key typ lbl =
    let
        labelWidth =
            120
    in
    Input.checkbox
        []
        { onChange =
            \d ->
                let
                    v =
                        if d then
                            "1"

                        else
                            "0"
                in
                EditPoint [ Point "" typ key Point.dataTypeInt v ]
        , checked =
            Point.getBool pts typ key
        , icon = Input.defaultCheckbox
        , label =
            if lbl /= "" then
                Input.labelLeft [ width (px labelWidth) ] <|
                    el [ alignRight ] <|
                        text <|
                            lbl
                                ++ ":"

            else
                Input.labelHidden ""
        }


viewIf : Bool -> Element msg -> Element msg
viewIf condition element =
    if condition then
        element

    else
        Element.none
