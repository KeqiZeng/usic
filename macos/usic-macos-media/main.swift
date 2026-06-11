import Foundation
import MediaPlayer

final class IpcClient {
    private let socketPath: String

    init(socketPath: String) {
        self.socketPath = socketPath
    }

    @discardableResult
    func send(_ request: [String: Any]) -> Data? {
        guard
            let data = try? JSONSerialization.data(withJSONObject: request),
            let line = String(data: data, encoding: .utf8)
        else {
            return nil
        }

        let fd = socket(AF_UNIX, SOCK_STREAM, 0)
        guard fd >= 0 else { return nil }
        defer { close(fd) }

        var address = sockaddr_un()
        address.sun_family = sa_family_t(AF_UNIX)
        let pathBytes = Array(socketPath.utf8) + [0]
        guard pathBytes.count <= MemoryLayout.size(ofValue: address.sun_path) else {
            return nil
        }

        withUnsafeMutablePointer(to: &address.sun_path) { pointer in
            pointer.withMemoryRebound(to: CChar.self, capacity: pathBytes.count) { buffer in
                for (index, byte) in pathBytes.enumerated() {
                    buffer[index] = CChar(bitPattern: byte)
                }
            }
        }

        let addressLength = socklen_t(MemoryLayout<sa_family_t>.size + pathBytes.count)
        let connected = withUnsafePointer(to: &address) { pointer in
            pointer.withMemoryRebound(to: sockaddr.self, capacity: 1) { socketAddress in
                Darwin.connect(fd, socketAddress, addressLength)
            }
        }
        guard connected == 0 else { return nil }

        let payload = Array((line + "\n").utf8)
        guard payload.withUnsafeBytes({ Darwin.write(fd, $0.baseAddress, payload.count) }) == payload.count else {
            return nil
        }

        var response = Data()
        var buffer = [UInt8](repeating: 0, count: 4096)
        while true {
            let count = Darwin.read(fd, &buffer, buffer.count)
            if count <= 0 { break }
            response.append(buffer, count: count)
            if buffer.prefix(count).contains(UInt8(ascii: "\n")) { break }
        }
        return response
    }

    func status() -> PlayerStatus? {
        guard let data = send(["type": "get_status"]) else { return nil }
        return try? JSONDecoder().decode(ServerResponse.self, from: data).playerStatus
    }
}

struct ServerResponse: Decodable {
    let responseStatus: String
    let data: ResponseData?

    enum CodingKeys: String, CodingKey {
        case responseStatus = "status"
        case data
    }

    var playerStatus: PlayerStatus? {
        guard responseStatus == "ok" else { return nil }
        return data?.status
    }
}

struct ResponseData: Decodable {
    let type: String
    let value: PlayerStatus?

    var status: PlayerStatus? {
        type == "status" ? value : nil
    }
}

struct PlayerStatus: Decodable {
    let current_track: String?
    let position_secs: UInt64
    let duration_secs: UInt64?
    let paused: Bool
}

final class MediaBridge {
    private let client: IpcClient

    init(client: IpcClient) {
        self.client = client
    }

    func start() {
        let center = MPRemoteCommandCenter.shared()
        center.playCommand.isEnabled = true
        center.pauseCommand.isEnabled = true
        center.togglePlayPauseCommand.isEnabled = true
        center.nextTrackCommand.isEnabled = true
        center.previousTrackCommand.isEnabled = true

        center.playCommand.addTarget { [weak self] _ in
            self?.send(["type": "set_paused", "paused": false])
            return .success
        }
        center.pauseCommand.addTarget { [weak self] _ in
            self?.send(["type": "set_paused", "paused": true])
            return .success
        }
        center.togglePlayPauseCommand.addTarget { [weak self] _ in
            self?.send(["type": "toggle_pause"])
            return .success
        }
        center.nextTrackCommand.addTarget { [weak self] _ in
            self?.send(["type": "next"])
            return .success
        }
        center.previousTrackCommand.addTarget { [weak self] _ in
            self?.send(["type": "prev"])
            return .success
        }

        updateNowPlaying()
        Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.updateNowPlaying()
        }
    }

    private func send(_ request: [String: Any]) {
        client.send(request)
        updateNowPlaying()
    }

    private func updateNowPlaying() {
        guard let status = client.status() else { return }
        guard let track = status.current_track else {
            MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
            MPNowPlayingInfoCenter.default().playbackState = .stopped
            return
        }

        var info: [String: Any] = [
            MPMediaItemPropertyTitle: (track as NSString).lastPathComponent,
            MPNowPlayingInfoPropertyElapsedPlaybackTime: Double(status.position_secs),
            MPNowPlayingInfoPropertyPlaybackRate: status.paused ? 0.0 : 1.0,
            MPNowPlayingInfoPropertyDefaultPlaybackRate: 1.0
        ]
        if let duration = status.duration_secs {
            info[MPMediaItemPropertyPlaybackDuration] = Double(duration)
        }

        let center = MPNowPlayingInfoCenter.default()
        center.nowPlayingInfo = info
        center.playbackState = status.paused ? .paused : .playing
    }
}

guard CommandLine.arguments.count == 2 else {
    FileHandle.standardError.write(Data("usage: usic-macos-media <socket-path>\n".utf8))
    exit(2)
}

let bridge = MediaBridge(client: IpcClient(socketPath: CommandLine.arguments[1]))
bridge.start()
RunLoop.main.run()
