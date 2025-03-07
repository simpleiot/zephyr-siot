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
import UI.Container as Container
import UI.Device as Device exposing (Device)
import UI.Nav as Nav
import UI.Page as PageUI
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
    { device : Device }


type Msg
    = NoOp


init : () -> ( Model, Effect Msg )
init () =
    ( { device = Device.classifyDevice 1024 768 }
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
view shared model =
    let
        device =
            Device.classifyDevice shared.windowWidth shared.windowHeight
    in
    PageUI.view
        { title = "Z-MR"
        , device = device
        , layout = PageUI.Standard Nav.Live -- Home page shows Live view by default
        , header = PageUI.header device "Testing"
        , content =
            [ welcomeCard device
            ]
        }


welcomeCard : Device -> Element Msg
welcomeCard device =
    Container.contentCard device
        "Z-MR Management Interface"
        [ PageUI.paragraph device
            PageUI.Body
            [ Font.center
            , Font.color Style.colors.gray
            , width (fill |> maximum (round (toFloat Device.breakpoints.tablet * 0.8))) -- 80% of tablet width
            , centerX
            , spacing 8
            ]
            [ Element.text "Welcome to the Z-MR management interface. Choose a section below to get started." ]
        , navigationButtons device
        ]


navigationButtons : Device -> Element Msg
navigationButtons device =
    let
        buttonLayout =
            case ( device.class, device.orientation ) of
                ( Device.Phone, Device.Portrait ) ->
                    column

                ( Device.Phone, Device.Landscape ) ->
                    wrappedRow

                _ ->
                    wrappedRow

        spacingValue =
            Device.responsiveSpacing device 16
    in
    buttonLayout
        [ spacing spacingValue
        , centerX
        , width fill
        , paddingEach { top = 16, right = 0, bottom = 0, left = 0 }
        ]
        [ navButton device Nav.Live "Live View" "Monitor system status and ATS states in real-time"
        , navButton device Nav.Settings "Settings" "Configure network settings and system parameters"
        , navButton device Nav.Events "Events" "View system events and notifications"
        ]


navButton : Device -> Nav.Route -> String -> String -> Element Msg
navButton device route label description =
    let
        buttonWidth =
            case ( device.class, device.orientation ) of
                ( Device.Phone, Device.Portrait ) ->
                    fill

                ( Device.Phone, Device.Landscape ) ->
                    fill |> minimum (round (toFloat device.width * 0.3)) |> maximum (round (toFloat device.width * 0.45))

                ( Device.Tablet, _ ) ->
                    fill |> minimum (round (toFloat Device.breakpoints.tablet * 0.25)) |> maximum (round (toFloat Device.breakpoints.tablet * 0.4))

                ( Device.Desktop, _ ) ->
                    fill |> minimum (round (toFloat Device.breakpoints.desktop * 0.2)) |> maximum (round (toFloat Device.breakpoints.desktop * 0.3))

        buttonHeight =
            case device.class of
                Device.Phone ->
                    px (round (toFloat Device.breakpoints.phone * 0.25))

                -- 25% of phone width for height
                _ ->
                    px (round (toFloat Device.breakpoints.tablet * 0.2))

        -- 20% of tablet width for height
    in
    link
        [ width buttonWidth
        ]
        { url = routeToUrl route
        , label =
            column
                [ spacing (round (toFloat (Device.responsiveSpacing device 12)))
                , padding (Device.responsiveSpacing device 24)
                , width fill
                , height buttonHeight
                , Background.color Style.colors.pale
                , Border.rounded 8
                , mouseOver [ Background.color Style.colors.light ]
                , transition { property = "background-color", duration = 150 }
                ]
                [ PageUI.text device PageUI.Large label
                , PageUI.paragraph device
                    PageUI.Small
                    [ Font.color Style.colors.gray ]
                    [ Element.text description ]
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
