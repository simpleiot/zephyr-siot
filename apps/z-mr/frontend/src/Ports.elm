port module Ports exposing (onWindowResize)

port onWindowResize : ({ width : Int, height : Int } -> msg) -> Sub msg 