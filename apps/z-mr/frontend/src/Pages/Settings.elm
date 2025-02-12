module Pages.Settings exposing (Model, Msg, page)

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
import Page exposing (Page)
import Route exposing (Route)
import Shared
import UI.Form as Form
import UI.Nav as Nav
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
    = NoOp
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
            [ spacing 32
            , padding 40
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
        , el [ Font.size 32, Font.bold, Font.color Style.colors.jet ] <| text "Settings"
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
