module Pages.Home_ exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Html.Attributes as Attr
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


type Tab
    = DashboardTab
    | SettingsTab


type alias Model =
    { points : Api.Data (List Point)
    , pointMods : List Point
    , blink : Bool
    , currentTab : Tab
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading [] False DashboardTab
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
    | ChangeTab Tab


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

        ChangeTab tab ->
            ( { model | currentTab = tab }
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
        column
            [ spacing 32
            , padding 40
            , width (fill |> maximum 1280)
            , height fill
            , centerX
            , Background.color Style.colors.pale
            ]
            [ header
            , deviceContent model
            ]
    }


header : Element Msg
header =
    row
        [ spacing 32
        , padding 24
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ image
            [ width (px 180)
            , alignLeft
            ]
            { src = "https://zonit.com/wp-content/uploads/2023/10/zonit-primary-rgb-300.png"
            , description = "Z-MR"
            }
        , el [ Font.size 32, Font.bold, Font.color Style.colors.jet ] <| text "Z-MR Dashboard"
        ]


h1 : String -> Element Msg
h1 txt =
    el
        [ Font.size 24
        , Font.semiBold
        , Font.color Style.colors.jet
        , paddingEach { top = 16, right = 0, bottom = 8, left = 0 }
        ]
    <|
        text txt


card : List (Element Msg) -> Element Msg
card content =
    column
        [ spacing 16
        , padding 24
        , width fill
        , height fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        content


tabButton : Tab -> Tab -> String -> Element Msg
tabButton currentTab tab label =
    Input.button
        [ padding 12
        , Border.roundEach { topLeft = 8, topRight = 8, bottomLeft = 0, bottomRight = 0 }
        , if currentTab == tab then
            Background.color Style.colors.white

          else
            Background.color Style.colors.pale
        , if currentTab == tab then
            Border.widthEach { bottom = 2, top = 0, left = 0, right = 0 }

          else
            Border.width 0
        , Border.color Style.colors.blue
        , mouseOver [ Background.color Style.colors.white ]
        , transition { property = "background-color", duration = 150 }
        ]
        { onPress = Just (ChangeTab tab)
        , label = text label
        }


tabs : Tab -> Element Msg
tabs currentTab =
    row
        [ spacing 4
        , width fill
        , Border.widthEach { bottom = 2, top = 0, left = 0, right = 0 }
        , Border.color Style.colors.ltgray
        ]
        [ tabButton currentTab DashboardTab "Dashboard"
        , tabButton currentTab SettingsTab "Settings"
        ]


deviceContent : Model -> Element Msg
deviceContent model =
    case model.points of
        Api.Loading ->
            el
                [ width fill
                , height fill
                , Background.color Style.colors.white
                , Border.rounded 12
                , padding 32
                , Font.center
                ]
            <|
                el [ centerX, centerY ] <|
                    text "Loading ..."

        Api.Success points ->
            let
                pointsMerge =
                    Point.updatePoints points model.pointMods
            in
            column [ spacing 24, width fill ]
                [ tabs model.currentTab
                , case model.currentTab of
                    DashboardTab ->
                        row [ spacing 24, width fill, height (px 300) ]
                            [ el [ width (fillPortion 3), height fill ] <| statusCard pointsMerge
                            , el [ width (fillPortion 2), height fill ] <| atsStateCard points model.blink
                            ]

                    SettingsTab ->
                        settingsCard pointsMerge (List.length model.pointMods > 0)
                ]

        Api.Failure httpError ->
            card
                [ el
                    [ Font.color Style.colors.red
                    , Font.size 16
                    , padding 16
                    , width fill
                    , Border.rounded 8
                    , Background.color (rgba 1 0 0 0.1)
                    ]
                  <|
                    text <|
                        "Lost connection: "
                            ++ Api.toUserFriendlyMessage httpError
                ]


statusCard : List Point -> Element Msg
statusCard points =
    let
        metricRow name value =
            row
                [ spacing 16
                , padding 16
                , width fill
                , Border.rounded 8
                , mouseOver [ Background.color Style.colors.pale ]
                , transition { property = "background-color", duration = 150 }
                ]
                [ el
                    [ Font.color Style.colors.gray
                    , width (px 120)
                    ]
                  <|
                    text name
                , el
                    [ Font.semiBold
                    , Font.color Style.colors.jet
                    ]
                  <|
                    text value
                ]

        data =
            [ { name = "Board", value = Point.getText points Point.typeBoard "0" }
            , { name = "Boot count", value = Point.getText points Point.typeBootCount "0" }
            , { name = "CPU Usage", value = Round.round 2 (Point.getFloat points Point.typeMetricSysCPUPercent "0") ++ "%" }
            , { name = "Uptime", value = Point.getText points Point.typeUptime "0" ++ "s" }
            , { name = "Temperature", value = Round.round 2 (Point.getFloat points Point.typeTemperature "0") ++ " °C" }
            , { name = "Fan 1 Speed", value = formatNumber (Point.getInt points "fanSpeed" "0") ++ " RPM" }
            , { name = "Fan 2 Speed", value = formatNumber (Point.getInt points "fanSpeed" "1") ++ " RPM" }
            ]
    in
    card
        [ h1 "System Status"
        , column [ spacing 4, width fill ] <|
            List.map (\d -> metricRow d.name d.value) data
        ]


settingsCard : List Point -> Bool -> Element Msg
settingsCard points edit =
    let
        staticIP =
            Point.getBool points Point.typeStaticIP ""

        inputStyle =
            [ width fill
            , padding 12
            , Border.width 1
            , Border.color Style.colors.ltgray
            , Border.rounded 8
            , focused [ Border.color Style.colors.blue ]
            , transition { property = "border-color", duration = 150 }
            ]
    in
    card
        [ h1 "Settings"
        , column [ spacing 24, Form.onEnterEsc (ApiPostPoints points) DiscardEdits ]
            [ inputText points "0" Point.typeDescription "Description" "desc" inputStyle
            , inputText points "0" "snmpServer" "SNMP Server" "IP address" inputStyle
            , inputCheckbox points "0" Point.typeStaticIP "Static IP" []
            , viewIf staticIP <|
                column [ spacing 16 ]
                    [ inputText points "0" Point.typeAddress "IP Addr" "ex: 10.0.0.23" inputStyle
                    , inputText points "0" Point.typeNetmask "Netmask" "ex: 255.255.255.0" inputStyle
                    , inputText points "0" Point.typeGateway "Gateway" "ex: 10.0.0.1" inputStyle
                    ]
            , viewIf edit <|
                row [ spacing 16 ]
                    [ Input.button
                        [ padding 12
                        , Border.rounded 8
                        , Background.color Style.colors.blue
                        , Font.color Style.colors.white
                        , mouseOver [ Background.color Style.colors.ltblue ]
                        , transition { property = "background-color", duration = 150 }
                        ]
                        { onPress = Just (ApiPostPoints points)
                        , label = text "Save Changes"
                        }
                    , Input.button
                        [ padding 12
                        , Border.rounded 8
                        , Background.color Style.colors.ltgray
                        , Font.color Style.colors.jet
                        , mouseOver [ Background.color Style.colors.darkgray ]
                        , transition { property = "background-color", duration = 150 }
                        ]
                        { onPress = Just DiscardEdits
                        , label = text "Discard"
                        }
                    ]
            ]
        ]


atsStateCard : List Point -> Bool -> Element Msg
atsStateCard pts blink =
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

        cell content =
            el
                [ paddingXY 8 12
                , centerX
                , centerY
                , Font.center
                ]
                content

        headerCell content =
            el
                [ paddingXY 8 12
                , centerX
                , centerY
                , Font.center
                , Font.semiBold
                , Font.color Style.colors.gray
                ]
                content
    in
    card
        [ h1 "ATS Status"
        , column [ height fill, width fill, spacing 0 ] <|
            [ table
                [ spacing 12
                , padding 16
                , width fill
                , height fill
                , Background.color Style.colors.pale
                , Border.rounded 8
                ]
                { data = tableData
                , columns =
                    { header = headerCell <| text "Side"
                    , width = shrink
                    , view = \r -> cell <| el [ Font.semiBold ] <| text r.side
                    }
                        :: List.map
                            (\i ->
                                { header = headerCell <| text <| String.fromInt (i + 1)
                                , width = fill
                                , view = \r -> cell <| (List.Extra.getAt i r.leds |> Maybe.withDefault none)
                                }
                            )
                            (List.range 0 5)
                }
            ]
        ]


inputText : List Point -> String -> String -> String -> String -> List (Attribute Msg) -> Element Msg
inputText pts key typ lbl placeholder styles =
    let
        labelWidth =
            120
    in
    Input.text
        styles
        { onChange =
            \d ->
                EditPoint [ Point typ key Point.dataTypeString d ]
        , text = Point.getText pts typ key
        , placeholder = Just <| Input.placeholder [] <| text placeholder
        , label =
            if lbl == "" then
                Input.labelHidden ""

            else
                Input.labelLeft
                    [ width (px labelWidth)
                    , Font.color Style.colors.gray
                    ]
                <|
                    el [ alignRight ] <|
                        text <|
                            lbl
                                ++ ":"
        }


inputCheckbox : List Point -> String -> String -> String -> List (Attribute Msg) -> Element Msg
inputCheckbox pts key typ lbl styles =
    let
        labelWidth =
            120
    in
    Input.checkbox
        styles
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


transition : { property : String, duration : Int } -> Attribute msg
transition { property, duration } =
    Element.htmlAttribute
        (Attr.style "transition"
            (property ++ " " ++ String.fromInt duration ++ "ms ease-in-out")
        )


formatNumber : Int -> String
formatNumber number =
    let
        numberString =
            String.fromInt (abs number)

        reversedDigits =
            String.toList numberString |> List.reverse

        groupedDigits =
            List.Extra.greedyGroupsOf 3 reversedDigits
                |> List.map String.fromList
                |> List.map String.reverse
                |> List.reverse
                |> String.join ","
    in
    if number < 0 then
        "-" ++ groupedDigits

    else
        groupedDigits
