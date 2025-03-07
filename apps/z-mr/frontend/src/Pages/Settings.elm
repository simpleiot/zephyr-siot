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
import UI.Device
import UI.Form as Form
import UI.Nav as Nav
import UI.Page as PageUI
import UI.Sanitize as Sanitize
import UI.Style as Style
import View exposing (View)


page : Shared.Model -> Route () -> Page Model Msg
page shared _ =
    Page.new
        { init = init
        , update = update
        , subscriptions = subscriptions
        , view = view shared
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


view : Shared.Model -> Model -> View Msg
view shared model =
    PageUI.view
        { title = "Z-MR Settings"
        , device = UI.Device.classifyDevice shared.windowWidth shared.windowHeight
        , layout = PageUI.Standard Nav.Settings
        , header = PageUI.header (UI.Device.classifyDevice shared.windowWidth shared.windowHeight) "Settings"
        , content = [ deviceContent shared model ]
        }


h1 : Shared.Model -> String -> Element Msg
h1 shared txt =
    paragraph
        [ Font.size (responsiveFontSize shared.windowWidth 24)
        , Font.semiBold
        , Font.color Style.colors.jet
        , paddingEach { top = 16, right = 0, bottom = 8, left = 0 }
        , width fill
        ]
        [ text txt ]


card : Shared.Model -> List (Element Msg) -> Element Msg
card shared content =
    column
        [ spacing (responsiveSpacing shared.windowWidth 16)
        , padding
            (if shared.windowWidth <= 768 then
                16

             else
                responsiveSpacing shared.windowWidth 24
            )
        , width fill
        , height fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        content


deviceContent : Shared.Model -> Model -> Element Msg
deviceContent shared model =
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
            settingsCard shared mockPoints False

        Api.Success points ->
            let
                pointsMerge =
                    Point.updatePoints points model.pointMods
            in
            settingsCard shared pointsMerge (List.length model.pointMods > 0)

        Api.Failure httpError ->
            card shared
                [ el
                    [ Font.color Style.colors.danger
                    , Font.size (responsiveFontSize shared.windowWidth 16)
                    , padding (responsiveSpacing shared.windowWidth 16)
                    , width fill
                    , Border.rounded 8
                    , Background.color (rgba 1 0 0 0.1)
                    ]
                  <|
                    text <|
                        "Lost connection: "
                            ++ Api.toUserFriendlyMessage httpError
                ]


settingsCard : Shared.Model -> List Point -> Bool -> Element Msg
settingsCard shared points edit =
    let
        staticIP =
            Point.getBool points Point.typeStaticIP ""

        inputStyle =
            [ width fill
            , padding
                (if shared.windowWidth <= 768 then
                    10

                 else
                    responsiveSpacing shared.windowWidth 12
                )
            , Border.width 1
            , Border.color Style.colors.gray
            , Border.rounded 8
            , focused [ Border.color Style.colors.primary ]
            , transition { property = "border-color", duration = 150 }
            , Font.size (responsiveFontSize shared.windowWidth 14)
            ]
    in
    card shared
        [ h1 shared "Settings"
        , column
            [ spacing (responsiveSpacing shared.windowWidth 24)
            , Form.onEnterEsc (ApiPostPoints points) DiscardEdits
            , width fill
            ]
            [ inputText shared points "0" Point.typeDescription "Description" "desc" inputStyle
            , inputText shared points "0" "snmpServer" "SNMP Server" "IP address" inputStyle
            , fanSettings shared points
            , inputCheckbox shared points "0" Point.typeStaticIP "Static IP" []
            , viewIf staticIP <|
                column [ spacing (responsiveSpacing shared.windowWidth 16), width fill ]
                    [ inputText shared points "0" Point.typeAddress "IP Addr" "ex: 10.0.0.23" inputStyle
                    , inputText shared points "0" Point.typeNetmask "Netmask" "ex: 255.255.255.0" inputStyle
                    , inputText shared points "0" Point.typeGateway "Gateway" "ex: 10.0.0.1" inputStyle
                    ]
            , viewIf edit <|
                (if shared.windowWidth <= 768 then
                    column

                 else
                    wrappedRow
                )
                    [ spacing (responsiveSpacing shared.windowWidth 16)
                    , width fill
                    ]
                    [ Input.button
                        [ padding (responsiveSpacing shared.windowWidth 12)
                        , Border.rounded 8
                        , Background.color Style.colors.primary
                        , Font.color Style.colors.white
                        , mouseOver [ Background.color Style.colors.light ]
                        , transition { property = "background-color", duration = 150 }
                        ]
                        { onPress = Just (ApiPostPoints points)
                        , label = text "Save Changes"
                        }
                    , Input.button
                        [ padding (responsiveSpacing shared.windowWidth 12)
                        , Border.rounded 8
                        , Background.color Style.colors.gray
                        , Font.color Style.colors.jet
                        , mouseOver [ Background.color Style.colors.dark ]
                        , transition { property = "background-color", duration = 150 }
                        ]
                        { onPress = Just DiscardEdits
                        , label = text "Discard"
                        }
                    ]
            ]
        ]


fanSettings : Shared.Model -> List Point -> Element Msg
fanSettings shared points =
    let
        device =
            UI.Device.classifyDevice shared.windowWidth shared.windowHeight

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
    column
        [ spacing (responsiveSpacing shared.windowWidth 24)
        , alignLeft
        , width fill
        ]
        [ inputOption shared
            points
            "0"
            ZPoint.typeFanMode
            "Fan mode"
            [ ( ZPoint.valueOff, "Off" )
            , ( ZPoint.valuePwm, "PWM" )
            , ( ZPoint.valueTach, "Tachometer" )
            , ( ZPoint.valueTemp, "Temperature" )
            ]
        , viewIf (mode /= ZPoint.valueOff) <|
            wrappedRow [ spacing 10, width fill, alignLeft ]
                [ inputFloat shared points "0" ZPoint.typeFanSetSpeed setting1Desc
                , el [ Font.size (UI.Device.responsiveFontSize device 14) ] (text speedUnits)
                ]
        , viewIf (mode /= ZPoint.valueOff) <|
            wrappedRow [ spacing 10, width fill, alignLeft ]
                [ inputFloat shared points "1" ZPoint.typeFanSetSpeed setting2Desc
                , el [ Font.size (UI.Device.responsiveFontSize device 14) ] (text speedUnits)
                ]
        ]


inputText : Shared.Model -> List Point -> String -> String -> String -> String -> List (Attribute Msg) -> Element Msg
inputText shared pts key typ lbl placeholder styles =
    column
        [ width fill
        , spacing 8
        ]
        [ if lbl /= "" then
            el
                [ Font.color Style.colors.gray
                , Font.size (responsiveFontSize shared.windowWidth 14)
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


inputCheckbox : Shared.Model -> List Point -> String -> String -> String -> List (Attribute Msg) -> Element Msg
inputCheckbox shared pts key typ lbl styles =
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
                , Font.size (responsiveFontSize shared.windowWidth 14)
                ]
            <|
                text lbl

          else
            none
        ]


blankMajicValue : String
blankMajicValue =
    "123BLANK123"


inputFloat : Shared.Model -> List Point -> String -> String -> String -> Element Msg
inputFloat shared pts key typ lbl =
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
                , Font.size (responsiveFontSize shared.windowWidth 14)
                ]
            <|
                text lbl

          else
            none
        , Input.text
            [ width (fill |> maximum 120)
            , padding (responsiveSpacing shared.windowWidth 12)
            , Border.width 1
            , Border.color Style.colors.gray
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


inputOption : Shared.Model -> List Point -> String -> String -> String -> List ( String, String ) -> Element Msg
inputOption shared pts key typ lbl options =
    column
        [ spacing 8
        , alignLeft
        , width fill
        ]
        [ if lbl /= "" then
            el
                [ Font.color Style.colors.gray
                , Font.size (responsiveFontSize shared.windowWidth 14)
                ]
            <|
                text (lbl ++ ":")

          else
            none
        , Input.radio
            [ spacing 6
            , alignLeft
            ]
            { onChange =
                \sel ->
                    EditPoint [ Point typ key Point.dataTypeString sel ]
            , label = Input.labelHidden lbl
            , selected = Just <| Point.getText pts typ key
            , options =
                List.map
                    (\opt ->
                        Input.option (Tuple.first opt) (text (Tuple.second opt))
                    )
                    options
            }
        ]


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


responsiveSpacing : Int -> Int -> Int
responsiveSpacing windowWidth base =
    if windowWidth <= 480 then
        base // 2

    else
        base


responsiveFontSize : Int -> Int -> Int
responsiveFontSize windowWidth base =
    if windowWidth <= 480 then
        base * 4 // 5

    else
        base



-- Default to mobile breakpoint for now. We'll need to pass actual window dimensions from the app level.
