module Pages.Home_ exposing (Model, Msg, page)

import Effect exposing (Effect)
import Element exposing (..)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Html.Attributes as Attr
import Page exposing (Page)
import Route exposing (Route)
import Shared
import Task
import UI.Nav as Nav
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
    {}


type Msg
    = NoOp


init : () -> ( Model, Effect Msg )
init () =
    ( {}
    , Task.succeed () |> Task.perform (\_ -> NoOp) |> Effect.sendCmd
    )


update : Msg -> Model -> ( Model, Effect Msg )
update msg model =
    case msg of
        NoOp ->
            ( model
            , Effect.none
            )



-- SUBSCRIPTIONS


subscriptions : Model -> Sub Msg
subscriptions _ =
    Sub.none



-- VIEW


view : Shared.Model -> Model -> View Msg
view shared _ =
    { title = "Z-MR"
    , attributes = []
    , element =
        column
            [ spacing (responsiveSpacing shared.windowWidth 32)
            , padding (responsiveSpacing shared.windowWidth 40)
            , width (fill |> maximum 1280)
            , height fill
            , centerX
            , Background.color Style.colors.pale
            ]
            [ header shared
            , welcomeCard shared
            ]
    }


header : Shared.Model -> Element Msg
header shared =
    column
        [ spacing 16
        , padding (responsiveSpacing shared.windowWidth 24)
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
        , el [ Font.size (responsiveFontSize shared.windowWidth 32), Font.bold, Font.color Style.colors.jet ] <| text "Welcome"
        ]


welcomeCard : Shared.Model -> Element Msg
welcomeCard shared =
    column
        [ spacing 24
        , padding (responsiveSpacing shared.windowWidth 32)
        , width fill
        , Background.color Style.colors.white
        , Border.rounded 12
        , Border.shadow { offset = ( 0, 2 ), size = 0, blur = 8, color = rgba 0 0 0 0.1 }
        ]
        [ el
            [ Font.size (responsiveFontSize shared.windowWidth 24)
            , Font.semiBold
            , Font.color Style.colors.jet
            , centerX
            ]
          <|
            text "Z-MR Management Interface"
        , paragraph
            [ Font.center
            , Font.color Style.colors.gray
            , width (fill |> maximum 600)
            , centerX
            , spacing 8
            ]
            [ text "Welcome to the Z-MR management interface. Choose a section below to get started." ]
        , wrappedRow
            [ spacing (responsiveSpacing shared.windowWidth 24)
            , centerX
            , width fill
            , paddingEach { top = 16, right = 0, bottom = 0, left = 0 }
            ]
            [ navButton shared Nav.Live "Live View" "Monitor system status and ATS states in real-time"
            , navButton shared Nav.Settings "Settings" "Configure network settings and system parameters"
            , navButton shared Nav.Events "Events" "View system events and notifications"
            ]
        ]


navButton : Shared.Model -> Nav.Route -> String -> String -> Element Msg
navButton shared route label description =
    link
        [ width (fill |> minimum 250 |> maximum 350)
        ]
        { url = routeToUrl route
        , label =
            column
                [ spacing 12
                , padding (responsiveSpacing shared.windowWidth 24)
                , width fill
                , height (px 160)
                , Background.color Style.colors.pale
                , Border.rounded 8
                , mouseOver [ Background.color Style.colors.ltgray ]
                , transition { property = "background-color", duration = 150 }
                ]
                [ el
                    [ Font.size (responsiveFontSize shared.windowWidth 20)
                    , Font.semiBold
                    , Font.color Style.colors.jet
                    ]
                  <|
                    text label
                , paragraph
                    [ Font.color Style.colors.gray
                    , Font.size (responsiveFontSize shared.windowWidth 14)
                    ]
                    [ text description ]
                ]
        }


routeToUrl : Nav.Route -> String
routeToUrl route =
    case route of
        Nav.Live ->
            "/live"

        Nav.Settings ->
            "/settings"

        Nav.Events ->
            "/events"


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


deviceWidth : Int
deviceWidth =
    480  -- Default to mobile breakpoint for now. We'll need to pass actual window dimensions from the app level.
