module Pages.Events exposing (Event, EventSeverity, Model, Msg, page)

import Api
import Api.Point as Point exposing (Point)
import Effect exposing (Effect)
import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Html.Attributes as Attr
import Http
import Route exposing (Route)
import Shared
import Time
import UI.Container as Container
import UI.Device as Device exposing (Device)
import UI.Nav as Nav
import UI.Page as PageUI
import UI.Style as Style
import View exposing (View)
import Page exposing (Page)


page : Shared.Model -> Route () -> Page Model Msg
page shared _ =
    Page.new
        { init = init shared
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
    , device : Device
    }


init : Shared.Model -> () -> ( Model, Effect Msg )
init shared () =
    ( { points = Api.Loading
      , events = mockEvents
      , device = Device.classifyDevice shared.windowWidth shared.windowHeight
      }
    , pointFetch
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
    = ApiRespPointList (Result Http.Error (List Point))
    | PollPointList


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

        PollPointList ->
            ( model
            , pointFetch
            )



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions _ =
    Time.every 3000 (\_ -> PollPointList)



-- VIEW


view : Model -> View Msg
view model =
    PageUI.view
        { title = "Z-MR Events"
        , device = model.device
        , layout = PageUI.Standard Nav.Events
        , header = PageUI.header model.device "Events"
        , content =
            [ deviceContent model.device model
            ]
        }


deviceContent : Device -> Model -> Element Msg
deviceContent device model =
    Container.contentCard device "System Events"
        [ column 
            [ spacing (Device.responsiveSpacing device 8)
            , width fill
            , height fill
            , scrollbarY
            , paddingEach 
                { top = 0
                , right = Device.responsiveSpacing device 8  -- Room for scrollbar
                , bottom = Device.responsiveSpacing device 8
                , left = 0
                }
            ] <|
            List.map (eventRow device) model.events
        ]


eventRow : Device -> Event -> Element Msg
eventRow device event =
    let
        { red, green, blue } =
            toRgb (severityColor event.severity)
            
        isPhone =
            device.class == Device.Phone
            
        spacingValue =
            Device.responsiveSpacing device (if isPhone then 4 else 8)
            
        metadataItem content color =
            PageUI.text device PageUI.Small content
                |> el [ Font.color color ]
                
        separator =
            PageUI.text device PageUI.Small "•"
                |> el 
                    [ Font.color Style.colors.gray
                    , paddingXY 4 0
                    ]
    in
    column
        [ spacing spacingValue
        , padding (Device.responsiveSpacing device (if isPhone then 8 else 12))
        , width fill
        , Border.rounded 8
        , Background.color (rgba red green blue 0.1)
        , mouseOver [ Background.color (rgba red green blue 0.15) ]
        , transition { property = "background-color", duration = 150 }
        ]
        [ wrappedRow
            [ spacing spacingValue
            , width fill
            ]
            [ metadataItem event.timestamp Style.colors.gray
            , separator
            , metadataItem event.type_ Style.colors.gray
            , separator
            , metadataItem (severityToString event.severity) (severityColor event.severity)
            ]
        , PageUI.paragraph device PageUI.Small
            [ Font.color Style.colors.jet
            , width fill
            , paddingEach { top = 4, right = 0, bottom = 0, left = 0 }
            ]
            [ Element.text event.message ]
        ]


severityColor : EventSeverity -> Color
severityColor severity =
    case severity of
        Info ->
            Style.colors.info

        Warning ->
            Style.colors.warning

        Error ->
            Style.colors.danger


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


pointFetch : Effect Msg
pointFetch =
    Effect.sendCmd <| Point.fetch { onResponse = ApiRespPointList }

