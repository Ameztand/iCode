#pragma once

class IGameState;
class BaseGame;

class State;
class Time;
class Msg;
class Logic;
class Renderer;

class PUGame;
class ATGame;
class EXGame;

struct Context {
    State& state;

    Logic& logic;
    Renderer& render;

    Msg& msg;
    Time& gameTime;//记录游戏时间

};

