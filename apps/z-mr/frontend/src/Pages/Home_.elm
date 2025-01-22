module Pages.Home_ exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Http
import List.Extra
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
    , blink : Bool
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading [] False
    , Effect.batch <|
        [ Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
        , Effect.sendCmd <| Task.perform Tick Time.now
        ]
    )


type Msg
    = NoOp
    | Tick Time.Posix
    | BlinkTick Time.Posix
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

        BlinkTick _ ->
            ( { model | blink = not model.blink }
            , Effect.none
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
    Sub.batch
        [ Time.every 3000 Tick
        , Time.every 500 BlinkTick
        ]



-- VIEW


view : Model -> View Msg
view model =
    { title = "Z-MR"
    , attributes = []
    , element =
        column [ spacing 20, padding 60, width (fill |> maximum 1280), height fill, centerX ]
            [ row [ spacing 32 ]
                [ image [ width (px 220) ]
                    { src = "https://zonit.com/wp-content/uploads/2023/10/zonit-primary-rgb-300.png"
                    , description = "Z-MR"
                    }
                , el [ Font.size 43, Font.bold ] <| text "Z-MR"
                ]
            , deviceContent model
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
                , h1 "Devices"
                , atsState points model.blink
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
            , { name = "CPU Usage", value = Round.round 2 (Point.getFloat points Point.typeMetricSysCPUPercent "0") ++ "%" }
            , { name = "Uptime", value = Point.getText points Point.typeUptime "0" ++ "s" }
            , { name = "Temperature", value = Round.round 2 (Point.getFloat points Point.typeTemperature "0") ++ " °C" }
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
    let
        staticIP =
            Point.getBool points Point.typeStaticIP ""
    in
    column [ spacing 15, Form.onEnterEsc (ApiPostPoints points) DiscardEdits ]
        [ inputText points "0" Point.typeDescription "Description" "desc"
        , inputText points "0" "snmpServer" "SNMP Server" "IP address"
        , inputCheckbox points "0" Point.typeStaticIP "Static IP"
        , viewIf staticIP <|
            column [ spacing 15 ]
                [ inputText points "0" Point.typeAddress "IP Addr" "ex: 10.0.0.23"
                , inputText points "0" Point.typeNetmask "Netmask" "ex: 255.255.255.0"
                , inputText points "0" Point.typeGateway "Gateway" "ex: 10.0.0.1"
                ]
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
        labelWidth =
            120
    in
    Input.text
        []
        { onChange =
            \d ->
                EditPoint [ Point typ key Point.dataTypeString d ]
        , text = Point.getText pts typ key
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
                EditPoint [ Point typ key Point.dataTypeInt v ]
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


type AtsState
    = Off
    | On
    | Active
    | Error


type AtsSide
    = A
    | B


pointToAtsState : List Point -> String -> String -> AtsState
pointToAtsState pts typ key =
    case Point.getInt pts typ key of
        0 ->
            Off

        1 ->
            On

        2 ->
            Active

        _ ->
            Error


atsStateToLed : AtsSide -> Bool -> AtsState -> Element Msg
atsStateToLed side blink state =
    let
        onLed =
            case side of
                A ->
                    circleGreen

                B ->
                    circleBlue
    in
    case ( state, blink ) of
        ( Off, _ ) ->
            circleGray

        ( On, _ ) ->
            onLed

        ( Error, _ ) ->
            circleRed

        ( Active, True ) ->
            onLed

        ( Active, False ) ->
            circleLtGray


atsState : List Point -> Bool -> Element Msg
atsState pts blink =
    let
        sideA =
            List.map
                (\i ->
                    pointToAtsState pts "atsA" (String.fromInt i) |> atsStateToLed A blink
                )
                (List.range 0 5)

        sideB =
            List.map
                (\i ->
                    pointToAtsState pts "atsB" (String.fromInt i) |> atsStateToLed B blink
                )
                (List.range 0 5)

        tableData =
            [ { side = "A", leds = sideA }
            , { side = "B", leds = sideB }
            ]

        cell =
            el [ paddingXY 5 5, centerX, centerY, Font.center ]
    in
    table [ width shrink, padding 3 ]
        { data = tableData
        , columns =
            { header = el [ Font.bold ] <| cell <| text "ATS"
            , width = shrink
            , view = \r -> cell <| text r.side
            }
                :: List.map
                    (\i ->
                        { header = cell <| text <| String.fromInt (i + 1)
                        , width = shrink
                        , view = \r -> cell <| (List.Extra.getAt i r.leds |> Maybe.withDefault none)
                        }
                    )
                    (List.range 0 5)
        }


circleRed : Element Msg
circleRed =
    circle Style.colors.ledred


circleBlue : Element Msg
circleBlue =
    circle Style.colors.ledblue


circleGreen : Element Msg
circleGreen =
    circle Style.colors.ledgreen


circleGray : Element Msg
circleGray =
    circle Style.colors.gray


circleLtGray : Element Msg
circleLtGray =
    circle Style.colors.ltgray


circle : Color -> Element Msg
circle color =
    el
        [ width (Element.px 20)
        , height (Element.px 20)
        , Background.color color
        , Border.rounded 10
        , centerX
        , centerY
        ]
        Element.none


viewIf : Bool -> Element msg -> Element msg
viewIf condition element =
    if condition then
        element

    else
        Element.none
