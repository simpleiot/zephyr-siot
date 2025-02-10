module Pages.Events exposing (Model, Msg, page)

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
import Time
import UI.Style as Style
import View exposing (View)
import UI.Nav as Nav


page : Shared.Model -> Route () -> Page Model Msg
page _ _ =
    Page.new
        { init = init
        , update = update
        , subscriptions = subscriptions
        , view = view
        }


-- INIT


type alias Event =
    { timestamp : String
    , type_ : String
    , message : String
    , severity : EventSeverity
    }


type EventSeverity
    = Info
    | Warning
    | Error


type alias Model =
    { points : Api.Data (List Point)
    , events : List Event
    }


init : () -> ( Model, Effect Msg )
init () =
    ( Model Api.Loading mockEvents
    , Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
    )


mockEvents : List Event
mockEvents =
    [ Event "2024-02-07 15:30:00" "System" "System started" Info
    , Event "2024-02-07 15:30:05" "Network" "Network connection established" Info
    , Event "2024-02-07 15:35:00" "ATS" "ATS A1 switched to source B" Warning
    , Event "2024-02-07 15:40:00" "Temperature" "Temperature above threshold: 40°C" Warning
    , Event "2024-02-07 15:45:00" "Network" "Connection lost" Error
    , Event "2024-02-07 15:46:00" "Network" "Connection restored" Info
    ]


type Msg
    = NoOp
    | ApiRespPointList (Result Http.Error (List Point))
    | Tick Time.Posix


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

        Tick _ ->
            ( model
            , Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }
            )


-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions _ =
    Time.every 3000 Tick


-- VIEW


view : Model -> View Msg
view model =
    { title = "Z-MR Events"
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
            , Nav.view Nav.Events
            , eventsContent model
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
        , el [ Font.size 32, Font.bold, Font.color Style.colors.jet ] <| text "Events"
        ]

h1 : String -> Element Msg
h1 txt =
    el 
        [ Font.size 24
        , Font.semiBold
        , Font.color Style.colors.jet
        , paddingEach { top = 16, right = 0, bottom = 8, left = 0 }
        ] 
        <| text txt

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

eventsContent : Model -> Element Msg
eventsContent model =
    card
        [ h1 "System Events"
        , column [ spacing 8, width fill ] <|
            List.map eventRow model.events
        ]

eventRow : Event -> Element Msg
eventRow event =
    let
        { red, green, blue } =
            toRgb (severityColor event.severity)
    in
    row 
        [ spacing 100
        , padding 16
        , width fill
        , Border.rounded 8
        , Background.color (rgba red green blue 0.1)
        , mouseOver [ Background.color (rgba red green blue 0.15) ]
        , transition { property = "background-color", duration = 150 }
        ]
        [ el 
            [ width (px 180)
            , Font.color Style.colors.gray
            ] 
            <| text event.timestamp
        , el 
            [ width (px 120)
            , Font.color (severityColor event.severity)
            , Font.bold
            ] 
            <| text (severityToString event.severity)
        , el 
            [ width (px 120)
            , Font.color Style.colors.gray
            ] 
            <| text event.type_
        , paragraph 
            [ Font.color Style.colors.jet
            , width fill
            ] 
            [ text event.message ]
        ]

severityColor : EventSeverity -> Color
severityColor severity =
    case severity of
        Info ->
            Style.colors.blue

        Warning ->
            Style.colors.orange

        Error ->
            Style.colors.red

severityToString : EventSeverity -> String
severityToString severity =
    case severity of
        Info ->
            "Info"

        Warning ->
            "Warning"

        Error ->
            "Error"

transition : { property : String, duration : Int } -> Attribute msg
transition { property, duration } =
    Element.htmlAttribute
        (Attr.style "transition"
            (property ++ " " ++ String.fromInt duration ++ "ms ease-in-out")
        )

colorComponents : Color -> ( Float, Float, Float )
colorComponents color =
    let
        { red, green, blue } =
            toRgb color
    in
    ( red, green, blue )
