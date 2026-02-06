# EOS Testing Project

A pure C++ test project for Epic Online Services (EOS) P2P/Relay functionality. This project proves that EOS networking works for Crab Game-style multiplayer party games.

## Features

- **Authentication** - Device ID login (anonymous), Developer auth, Epic Account
- **Lobbies** - Create, join, search, manage multiplayer lobbies
- **P2P Networking** - NAT traversal, relay fallback, reliable/unreliable packets
- **Voice Chat** - Room-based voice, push-to-talk, volume controls
- **Matchmaking** - Session creation, skill-based matching, quick-play

## Project Structure

```
EOS-testing/
├── CMakeLists.txt              # Main build configuration
├── include/
│   └── eos_testing/
│       ├── eos_testing.hpp     # Main include header
│       ├── core/
│       │   └── platform.hpp    # EOS SDK initialization
│       ├── auth/
│       │   └── auth_manager.hpp
│       ├── lobby/
│       │   └── lobby_manager.hpp
│       ├── p2p/
│       │   └── p2p_manager.hpp
│       ├── voice/
│       │   └── voice_manager.hpp
│       └── matchmaking/
│           └── matchmaking_manager.hpp
├── src/
│   ├── core/
│   ├── auth/
│   ├── lobby/
│   ├── p2p/
│   ├── voice/
│   └── matchmaking/
├── tests/
│   └── test_client.cpp         # Full feature test
├── examples/
│   └── simple_lobby.cpp        # Minimal lobby example
└── external/
    └── eos-sdk/                # Place EOS SDK here
```

## Quick Start

### 1. Get EOS SDK

Download the EOS SDK from Epic Developer Portal:
https://dev.epicgames.com/portal

Extract to `./external/eos-sdk/` so the structure is:
```
external/eos-sdk/
├── Include/
│   └── eos_sdk.h
├── Lib/
│   └── EOSSDK-Win64-Shipping.lib
└── Bin/
    └── EOSSDK-Win64-Shipping.dll
```

### 2. Configure Your Product

Create an application at https://dev.epicgames.com/portal and get:
- Product ID
- Sandbox ID  
- Deployment ID
- Client ID
- Client Secret

### 3. Build

```bash
# Create build directory
mkdir build && cd build

# Configure (Windows)
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Run test client
./bin/Release/eos_test_client.exe
```

### 4. Stub Mode

If you don't have the EOS SDK installed, the project builds in **stub mode**. All functionality works with simulated responses, perfect for:
- Testing your game logic
- CI/CD pipelines
- Development without network

## Usage Examples

### Basic Initialization

```cpp
#include "eos_testing/eos_testing.hpp"

int main() {
    eos_testing::PlatformConfig config;
    config.product_name = "My Game";
    config.product_version = "1.0.0";
    config.product_id = "xxx";
    config.sandbox_id = "xxx";
    config.deployment_id = "xxx";
    config.client_id = "xxx";
    config.client_secret = "xxx";
    
    eos_testing::initialize(config, [](bool success, const std::string& msg) {
        std::cout << "EOS: " << msg << "\n";
    });
    
    // Game loop
    while (running) {
        eos_testing::tick();  // Must call every frame!
        // ... your game code ...
    }
    
    eos_testing::shutdown();
}
```

### Login with Device ID

```cpp
auto& auth = eos_testing::AuthManager::instance();

auth.login_device_id("PlayerName", [](const eos_testing::AuthResult& result) {
    if (result.success) {
        std::cout << "Logged in as: " << result.display_name << "\n";
    }
});
```

### Create and Join Lobbies

```cpp
auto& lobby = eos_testing::LobbyManager::instance();

// Host creates lobby
eos_testing::CreateLobbyOptions options;
options.lobby_name = "Fun Game!";
options.max_members = 8;
options.attributes["map"] = "arena";

lobby.create_lobby(options, [](bool success, const std::string& id, const std::string& err) {
    std::cout << "Lobby ID: " << id << "\n";
});

// Others join
lobby.join_lobby("lobby-id-here", [](bool success, const auto& info, const auto& err) {
    std::cout << "Joined: " << info.lobby_name << "\n";
});
```

### P2P Networking

```cpp
auto& p2p = eos_testing::P2PManager::instance();

// Initialize
eos_testing::P2PConfig config;
config.socket_name = "GameSocket";
p2p.initialize(config);
p2p.accept_connections();

// Set up packet handler
p2p.on_packet_received = [](const eos_testing::IncomingPacket& packet) {
    // Handle game data
};

// Send packets
struct PlayerPos { float x, y, z; };
PlayerPos pos = {10.0f, 5.0f, 20.0f};

// Unreliable for position updates (fast, may drop)
p2p.broadcast_packet(&pos, sizeof(pos), 0, 
    eos_testing::PacketReliability::UnreliableUnordered);

// Reliable for important events (guaranteed delivery)
p2p.send_packet(peer_id, &event, sizeof(event), 1,
    eos_testing::PacketReliability::ReliableOrdered);

// Process incoming (call every frame)
p2p.receive_packets();
```

### Voice Chat

```cpp
auto& voice = eos_testing::VoiceManager::instance();

voice.initialize();
voice.join_room("lobby-123", [](bool success, const std::string& room) {
    std::cout << "Joined voice room\n";
});

// Push-to-talk
voice.set_input_mode(eos_testing::VoiceInputMode::PushToTalk);
voice.set_push_to_talk(key_pressed);  // Call when PTT key state changes

// Mute controls
voice.set_self_mute(true);
voice.set_participant_mute(other_player, true);
voice.set_participant_volume(other_player, 0.5f);
```

## Architecture

### Module Dependencies

```
Platform (core)
    ↓
AuthManager ←── All other modules require authentication
    ↓
┌───────────┬───────────┬───────────┐
│ LobbyMgr  │  P2PMgr   │ VoiceMgr  │
└───────────┴───────────┴───────────┘
         ↓
   MatchmakingMgr
```

### Thread Safety

- All managers use mutex-protected internal state
- Callbacks are invoked on the main thread (during `tick()`)
- Safe to call from game thread

## Crab Game Integration Pattern

For a Crab Game-style party game:

1. **Menu** → Player logs in with Device ID
2. **Lobby Browser** → Search/create lobbies
3. **Lobby** → Players gather, set ready, chat
4. **Game Start** → Host starts match, P2P mesh forms
5. **Gameplay** → Broadcast positions, sync physics
6. **Round End** → Return to lobby

## Requirements

- CMake 3.20+
- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)
- EOS SDK (optional - runs in stub mode without it)

## License

This is a test/proof-of-concept project. Use as reference for your own EOS integration.
