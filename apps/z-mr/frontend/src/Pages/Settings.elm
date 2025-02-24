module Pages.Settings exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Api.ZPoint as ZPoint
import Effect exposing (Effect)
import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Html.Attributes as Attr
import Http
import Page exposing (Page)
import Route exposing (Route)
import Shared
import UI.Form as Form
import UI.Nav as Nav
import UI.Sanitize as Sanitize
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
    , Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
    )


type Msg
    = ApiRespPointList (Result Http.Error (List Point))
    | ApiRespPointPost (Result Http.Error Point.Resp)
    | EditPoint (List Point)
    | ApiPostPoints (List Point)
    | DiscardEdits


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
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
    Sub.none



-- VIEW


view : Model -> View Msg
view model =
    { title = "Z-MR Settings"
    , attributes = []
    , element =
        column
            [ spacing (responsiveSpacing 32)
            , padding (responsiveSpacing 40)
            , width (fill |> maximum 1280)
            , height fill
            , centerX
            , Background.color Style.colors.pale
            ]
            [ header
            , Nav.view Nav.Settings
            , deviceContent model
            ]
    }


header : Element Msg
header =
    column
        [ spacing 16
        , padding (responsiveSpacing 24)
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ image
            [ width (fill |> maximum 180)
            , alignLeft
            ]
            { src = "https://zonit.com/wp-content/uploads/2023/10/zonit-primary-rgb-300.png"
            , description = "Z-MR"
            }
        , el [ Font.size (responsiveFontSize 32), Font.bold, Font.color Style.colors.jet ] <| text "Settings"
        ]


h1 : String -> Element Msg
h1 txt =
    el
        [ Font.size (responsiveFontSize 24)
        , Font.semiBold
        , Font.color Style.colors.jet
        , paddingEach { top = 16, right = 0, bottom = 8, left = 0 }
        ]
    <|
        text txt


card : List (Element Msg) -> Element Msg
card content =
    column
        [ spacing (responsiveSpacing 16)
        , padding (responsiveSpacing 24)
        , width fill
        , height fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        content


deviceContent : Model -> Element Msg
deviceContent model =
    case model.points of
        Api.Loading ->
            let
                mockPoints =
                    [ Point Point.typeDescription "0" Point.dataTypeString "Mock Device 1"
                    , Point "snmpServer" "0" Point.dataTypeString "10.0.0.1"
                    , Point Point.typeStaticIP "0" Point.dataTypeInt "1"
                    , Point Point.typeAddress "0" Point.dataTypeString "192.168.1.100"
                    , Point Point.typeNetmask "0" Point.dataTypeString "255.255.255.0"
                    , Point Point.typeGateway "0" Point.dataTypeString "192.168.1.1"
                    ]
            in
            settingsCard mockPoints False

        Api.Success points ->
            let
                pointsMerge =
                    Point.updatePoints points model.pointMods
            in
            settingsCard pointsMerge (List.length model.pointMods > 0)

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


settingsCard : List Point -> Bool -> Element Msg
settingsCard points edit =
    let
        staticIP =
            Point.getBool points Point.typeStaticIP ""

        inputStyle =
            [ width fill
            , padding (responsiveSpacing 12)
            , Border.width 1
            , Border.color Style.colors.ltgray
            , Border.rounded 8
            , focused [ Border.color Style.colors.blue ]
            , transition { property = "border-color", duration = 150 }
            ]
    in
    card
        [ h1 "Settings"
        , column [ spacing (responsiveSpacing 24), Form.onEnterEsc (ApiPostPoints points) DiscardEdits ]
            [ inputText points "0" Point.typeDescription "Description" "desc" inputStyle
            , inputText points "0" "snmpServer" "SNMP Server" "IP address" inputStyle
            , fanSettings points
            , inputCheckbox points "0" Point.typeStaticIP "Static IP" []
            , viewIf staticIP <|
                column [ spacing (responsiveSpacing 16) ]
                    [ inputText points "0" Point.typeAddress "IP Addr" "ex: 10.0.0.23" inputStyle
                    , inputText points "0" Point.typeNetmask "Netmask" "ex: 255.255.255.0" inputStyle
                    , inputText points "0" Point.typeGateway "Gateway" "ex: 10.0.0.1" inputStyle
                    ]
            , viewIf edit <|
                wrappedRow [ spacing (responsiveSpacing 16), width fill ]
                    [ Input.button
                        [ padding (responsiveSpacing 12)
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
                        [ padding (responsiveSpacing 12)
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


fanSettings : List Point -> Element Msg
fanSettings points =
    let
        mode =
            Point.getText points ZPoint.typeFanMode "0"

        speedUnits =
            case mode of
                "pwm" ->
                    "%"

                "tach" ->
                    "RPM"

                "temp" ->
                    "°C"

                _ ->
                    ""

        setting1Desc =
            case mode of
                "temp" ->
                    "Fan start temp"

                _ ->
                    "Fan 1 Speed"

        setting2Desc =
            case mode of
                "temp" ->
                    "Fan max temp"

                _ ->
                    "Fan 2 Speed"
    in
    column [ spacing (responsiveSpacing 24) ]
        [ inputOption points
            "0"
            ZPoint.typeFanMode
            "Fan mode"
            [ ( ZPoint.valueOff, "Off" )
            , ( ZPoint.valuePwm, "PWM" )
            , ( ZPoint.valueTach, "Tachometer" )
            , ( ZPoint.valueTemp, "Temperature" )
            ]
        , viewIf (mode /= ZPoint.valueOff) <|
            wrappedRow [ spacing 10, width fill ] [ inputFloat points "0" ZPoint.typeFanSetSpeed setting1Desc, text speedUnits ]
        , viewIf (mode /= ZPoint.valueOff) <|
            wrappedRow [ spacing 10, width fill ] [ inputFloat points "1" ZPoint.typeFanSetSpeed setting2Desc, text speedUnits ]
        ]


inputText : List Point -> String -> String -> String -> String -> List (Attribute Msg) -> Element Msg
inputText pts key typ lbl placeholder styles =
    column
        [ width fill
        , spacing 8
        ]
        [ if lbl /= "" then
            el
                [ Font.color Style.colors.gray
                , Font.size (responsiveFontSize 14)
                ]
            <|
                text lbl
          else
            none
        , Input.text
            styles
            { onChange =
                \d ->
                    EditPoint [ Point typ key Point.dataTypeString d ]
            , text = Point.getText pts typ key
            , placeholder = Just <| Input.placeholder [] <| text placeholder
            , label = Input.labelHidden ""
            }
        ]


inputCheckbox : List Point -> String -> String -> String -> List (Attribute Msg) -> Element Msg
inputCheckbox pts key typ lbl styles =
    row
        [ spacing 8
        , width fill
        ]
        [ Input.checkbox
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
            , checked = Point.getBool pts typ key
            , icon = Input.defaultCheckbox
            , label = Input.labelHidden ""
            }
        , if lbl /= "" then
            el
                [ Font.color Style.colors.gray
                , Font.size (responsiveFontSize 14)
                ]
            <|
                text lbl
          else
            none
        ]


blankMajicValue : String
blankMajicValue =
    "123BLANK123"


inputFloat : List Point -> String -> String -> String -> Element Msg
inputFloat pts key typ lbl =
    let
        currentText =
            Point.getText pts typ key

        currentValue =
            case currentText of
                "" ->
                    ""

                "123BLANK123" ->
                    ""

                "-" ->
                    "-"

                _ ->
                    Sanitize.float currentText
    in
    column
        [ width fill
        , spacing 8
        ]
        [ if lbl /= "" then
            el
                [ Font.color Style.colors.gray
                , Font.size (responsiveFontSize 14)
                ]
            <|
                text lbl
          else
            none
        , Input.text
            [ width (fill |> maximum 120)
            , padding (responsiveSpacing 12)
            , Border.width 1
            , Border.color Style.colors.ltgray
            , Border.rounded 8
            ]
            { onChange =
                \d ->
                    let
                        v =
                            if d == "" then
                                blankMajicValue
                            else if d == "-" then
                                "-"
                            else
                                Sanitize.float d
                    in
                    EditPoint [ Point typ key Point.dataTypeFloat v ]
            , text = currentValue
            , placeholder = Nothing
            , label = Input.labelHidden ""
            }
        ]


inputOption : List Point -> String -> String -> String -> List ( String, String ) -> Element Msg
inputOption pts key typ lbl options =
    let
        labelWidth =
            200
    in
    Input.radio
        [ spacing 6 ]
        { onChange =
            \sel ->
                EditPoint [ Point typ key Point.dataTypeString sel ]
        , label =
            Input.labelLeft [ padding 12, width (px labelWidth) ] <|
                el [ alignRight ] <|
                    text <|
                        lbl
                            ++ ":"
        , selected = Just <| Point.getText pts typ key
        , options =
            List.map
                (\opt ->
                    Input.option (Tuple.first opt) (text (Tuple.second opt))
                )
                options
        }


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


responsiveSpacing : Int -> Int
responsiveSpacing base =
    if deviceWidth <= 480 then
        base // 2
    else
        base


responsiveFontSize : Int -> Int
responsiveFontSize base =
    if deviceWidth <= 480 then
        base * 4 // 5
    else
        base


deviceWidth : Int
deviceWidth =
    Element.classifyDevice (Element.width fill) |> .width
