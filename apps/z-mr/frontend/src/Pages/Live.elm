module Pages.Live exposing (Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Api.ZPoint as ZPoint
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
import Time
import UI.Device as Device exposing (Device)
import UI.Nav as Nav
import UI.Style as Style
import View exposing (View)
import UI.Page as PageUI


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
    , blink : Bool
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading False
    , Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
    )


type Msg
    = PollPointList
    | BlinkTick
    | ApiRespPointList (Result Http.Error (List Point))


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
        PollPointList ->
            ( model
            , pointFetch
            )

        BlinkTick ->
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
        [ Time.every 3000 (\_ -> PollPointList)
        , Time.every 500 (\_ -> BlinkTick)
        ]



-- VIEW


view : Shared.Model -> Model -> View Msg
view shared model =
    let
        device =
            Device.classifyDevice shared.windowWidth shared.windowHeight
    in
    PageUI.view
        { title = "Z-MR Live View"
        , device = device
        , layout = PageUI.Standard Nav.Live
        , header = PageUI.header device "Live View"
        , content =
            [ deviceContent device model
            ]
        }


header : Device -> Element Msg
header device =
    let
        logoSize =
            case device.class of
                Device.Phone ->
                    90
                
                Device.Tablet ->
                    if device.orientation == Device.Portrait then
                        120
                    else
                        140
                
                Device.Desktop ->
                    160

        headerFontSize =
            Device.responsiveFontSize device 32
    in
    column
        [ spacing (Device.responsiveSpacing device 16)
        , padding (Device.responsiveSpacing device 24)
        , width fill
        , clipX
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ row
            [ width fill
            , spacing (Device.responsiveSpacing device 16)
            ]
            [ image
                [ width (px logoSize)
                , alignLeft
                ]
                { src = "https://zonit.com/wp-content/uploads/2023/10/zonit-primary-rgb-300.png"
                , description = "Z-MR"
                }
            , paragraph 
                [ Font.size headerFontSize
                , Font.bold
                , Font.color Style.colors.jet
                , width fill
                ] 
                [ text "Live View" ]
            ]
        ]


h1 : Device -> String -> Element Msg
h1 device txt =
    paragraph
        [ Font.size (Device.responsiveFontSize device 24)
        , Font.semiBold
        , Font.color Style.colors.jet
        , paddingEach { top = 16, right = 0, bottom = 8, left = 0 }
        , width fill
        ]
    <|
        [ text txt ]


card : Device -> List (Element Msg) -> Element Msg
card device content =
    column
        [ spacing (Device.responsiveSpacing device 16)
        , padding (Device.responsiveSpacing device 24)
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        content


deviceContent : Device -> Model -> Element Msg
deviceContent device model =
    let
        contentLayout =
            case device.class of
                Device.Phone ->
                    column 
                        [ spacing (Device.responsiveSpacing device 16)
                        , width fill
                        , centerX
                        ]
                
                Device.Tablet ->
                    column 
                        [ spacing (Device.responsiveSpacing device 24)
                        , width fill
                        , centerX
                        ]
                
                Device.Desktop ->
                    column 
                        [ spacing (Device.responsiveSpacing device 24)
                        , width (fill |> maximum Device.breakpoints.maxContentWidth)
                        , centerX
                        ]

        cardWrapper content =
            el 
                [ width fill
                , alignTop
                ] 
                content
    in
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
            contentLayout
                [ cardWrapper (statusCard device mockPoints)
                , cardWrapper (atsStateCard device mockPoints model.blink)
                ]

        Api.Success points ->
            contentLayout
                [ cardWrapper (statusCard device points)
                , cardWrapper (atsStateCard device points model.blink)
                ]

        Api.Failure httpError ->
            contentLayout
                [ cardWrapper (errorCard device httpError) ]


statusCard : Device -> List Point -> Element Msg
statusCard device points =
    let
        spacingValue =
            Device.responsiveSpacing device 16
            
        fontSize =
            Device.responsiveFontSize device 14
    in
    column
        [ spacing (Device.responsiveSpacing device 16)
        , padding (Device.responsiveSpacing device 24)
        , width fill
        , height fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ h1 device "System Status"
        , column [ spacing spacingValue, width fill ] <|
            List.map (statusRow device) points
        ]


statusRow : Device -> Point -> Element Msg
statusRow device point =
    let
        fontSize =
            Device.responsiveFontSize device 14
            
        label =
            case point.typ of
                "board" -> "Board"
                "bootCount" -> "Boot Count"
                "metricSysCPUPercent" -> "CPU Usage"
                "uptime" -> "Uptime"
                "temperature" -> "Temperature"
                _ -> point.typ
                
        value =
            case point.typ of
                "metricSysCPUPercent" -> 
                    Round.round 2 (Point.getFloat [ point ] point.typ point.key) ++ "%"
                "uptime" -> 
                    Point.getText [ point ] point.typ point.key ++ "s"
                _ -> 
                    Point.getText [ point ] point.typ point.key
    in
    row
        [ spacing (Device.responsiveSpacing device 16)
        , width fill
        , Font.size fontSize
        ]
        [ el 
            [ width (fillPortion 1)
            , Font.color Style.colors.gray 
            ] 
            (text label)
        , el 
            [ width (fillPortion 2)
            , Font.color Style.colors.jet
            ] 
            (text value)
        ]


errorCard : Device -> Http.Error -> Element Msg
errorCard device httpError =
    column
        [ spacing (Device.responsiveSpacing device 16)
        , padding (Device.responsiveSpacing device 24)
        , width fill
        , height fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ el
            [ Font.color Style.colors.danger
            , Font.size (Device.responsiveFontSize device 16)
            , padding (Device.responsiveSpacing device 16)
            , width fill
            , Border.rounded 8
            , Background.color (rgba 1 0 0 0.1)
            ]
            (PageUI.paragraph device PageUI.Body [] [ text ("Lost connection: " ++ Api.toUserFriendlyMessage httpError) ])
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


atsStateCard : Device -> List Point -> Bool -> Element Msg
atsStateCard device pts blink =
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
                [ paddingXY (Device.responsiveSpacing device 4) (Device.responsiveSpacing device 8)
                , centerX
                , centerY
                , Font.center
                , Font.size (Device.responsiveFontSize device 14)
                ]
                content

        headerCell content =
            el
                [ paddingXY (Device.responsiveSpacing device 4) (Device.responsiveSpacing device 8)
                , centerX
                , centerY
                , Font.center
                , Font.semiBold
                , Font.color Style.colors.gray
                , Font.size (Device.responsiveFontSize device 14)
                ]
                content
    in
    card device
        [ h1 device "ATS Status"
        , column [ width fill, spacing 0 ] <|
            [ table
                [ spacing (Device.responsiveSpacing device 8)
                , padding (Device.responsiveSpacing device 4)
                , width fill
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


pointFetch : Effect Msg
pointFetch =
    Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }


formatOnOff : Int -> String
formatOnOff number =
    case number of
        1 ->
            "On"

        _ ->
            "Off"


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


responsiveSpacing : Int -> Int -> Int
responsiveSpacing windowWidth base =
    if windowWidth <= 428 then
        base // 2
    else
        base


responsiveFontSize : Int -> Int -> Int
responsiveFontSize windowWidth base =
    if windowWidth <= 428 then
        base * 4 // 5
    else
        base


deviceWidth : Int
deviceWidth =
    480  -- Default to mobile breakpoint for now. We'll need to pass actual window dimensions from the app level.
