module Pages.Live exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Html.Attributes as Attr
import Http
import List.Extra
import Page exposing (Page)
import Round
import Route exposing (Route)
import Shared
import Task
import Time
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
    , blink : Bool
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading False
    , Effect.batch <|
        [ Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
        , Effect.sendCmd <| Task.perform Tick Time.now
        ]
    )


type Msg
    = Tick Time.Posix
    | BlinkTick Time.Posix
    | ApiRespPointList (Result Http.Error (List Point))


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
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
    { title = "Z-MR Live View"
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
            , Nav.view Nav.Live
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
        , el [ Font.size 32, Font.bold, Font.color Style.colors.jet ] <| text "Live View"
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
                    [ Point Point.typeBoard "0" Point.dataTypeString "ZMR-1"
                    , Point Point.typeBootCount "0" Point.dataTypeString "42"
                    , Point Point.typeMetricSysCPUPercent "0" Point.dataTypeFloat "25.5"
                    , Point Point.typeUptime "0" Point.dataTypeString "3600"
                    , Point Point.typeTemperature "0" Point.dataTypeFloat "35.2"
                    ]
            in
            row [ spacing 24, width fill ]
                [ el [ width (fillPortion 3) ] <| statusCard mockPoints
                , el [ width (fillPortion 2) ] <| atsStateCard mockPoints model.blink
                ]

        Api.Success points ->
            row [ spacing 24, width fill ]
                [ el [ width (fillPortion 3) ] <| statusCard points
                , el [ width (fillPortion 2) ] <| atsStateCard points model.blink
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
            ]
    in
    card
        [ h1 "System Status"
        , column [ spacing 4, width fill ] <|
            List.map (\d -> metricRow d.name d.value) data
        ]


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


transition : { property : String, duration : Int } -> Attribute msg
transition { property, duration } =
    Element.htmlAttribute
        (Attr.style "transition"
            (property ++ " " ++ String.fromInt duration ++ "ms ease-in-out")
        )
