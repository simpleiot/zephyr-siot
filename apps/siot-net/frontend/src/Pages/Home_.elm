module Pages.Home_ exposing (Model, Msg, page)

import Effect exposing (Effect)
import Element exposing (..)
import Element.Font as Font
import Page exposing (Page)
import Route exposing (Route)
import Shared
import Time
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


type alias Point =
    { typ : String
    , key : String
    , time : Time.Posix
    , dataType : String
    , data : String
    }


type alias Model =
    {}


init : () -> ( Model, Effect Msg )
init () =
    ( {}
    , Effect.none
    )



-- UPDATE


type Msg
    = NoOp


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


view : Model -> View Msg
view _ =
    { title = "Z-MR"
    , attributes = []
    , element =
        column [ spacing 20, padding 60, width (fill |> maximum 1280), height fill, centerX ]
            [ row [ spacing 32 ]
                [ image []
                    { src = "https://zonit.com/wp-content/uploads/2023/10/zonit-primary-rgb-300.png"
                    , description = "zonit logo"
                    }
                , el [ Font.size 60, Font.bold ] <| text "Z-MR"
                ]
            , h1 "Status"
            , column [ spacing 10, padding 20 ]
                [ text "• Board: "
                , text "• Boot count: "
                , text "• CPU Usage: "
                , text "• Temperature: "
                ]
            , h1 "Settings"
            , h1 "Devices"
            ]
    }


h1 : String -> Element Msg
h1 txt =
    el [ Font.size 32, Font.bold ] <| text txt
