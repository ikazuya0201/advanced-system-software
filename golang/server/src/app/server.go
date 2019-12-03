package main

import (
    "log"
    "net/http"
    "encoding/json"
)


func main() {
    handler := NewTestHandler(0,0)
    http.Handle("/", http.StripPrefix("/", http.FileServer(http.Dir("/code/files"))))
    http.Handle("/unko", handler)
	log.Fatal(http.ListenAndServe(":8000", nil))
}

type Position struct {
    X int `json:"x"`
    Y int `json:"y"`
}


type TestHandler struct {
    pos_x int
    pos_y int
}

func NewTestHandler(pos_x int, pos_y int) *TestHandler {
    handler := new(TestHandler)
    handler.pos_x = pos_x
    handler.pos_y = pos_y
    return handler
}

func (handler *TestHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
    pos := Position{handler.pos_x, handler.pos_y}
    pos_json, _ := json.Marshal(pos)
    handler.pos_x += 10
    handler.pos_y += 10
    w.Header().Set("Content-Type", "application/json")
    w.Write(pos_json)
}
