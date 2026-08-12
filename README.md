# WHIP Custom Simulcast

[![Build](https://github.com/yuyutti/WHIP-Custom-Simulcast/actions/workflows/push.yaml/badge.svg?branch=main)](https://github.com/yuyutti/WHIP-Custom-Simulcast/actions/workflows/push.yaml)
[![Release](https://img.shields.io/github/v/release/yuyutti/WHIP-Custom-Simulcast?include_prereleases)](https://github.com/yuyutti/WHIP-Custom-Simulcast/releases)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL--2.0--or--later-blue.svg)](LICENSE)

[English](README.en.md) | 日本語

OBS Studio 32.2.1の標準WHIP Simulcast Outputを維持したまま、各サブレイヤーの解像度、フレームレート、ビットレートを個別設定するOBSプラグインです。

## OBS標準の「均等分割」から、レイヤーを完全指定へ

OBS標準のWHIP Simulcastで指定できるのは合計レイヤー数だけです。サブレイヤーの解像度とビットレートは
メイン出力から自動的に均等分割され、FPSはメイン出力と同じ値になります。この動作は
[OBS Studio 32.2.1の標準実装](https://github.com/obsproject/obs-studio/blob/32.2.1/frontend/utility/WHIPSimulcastEncoders.hpp)に基づきます。

- 2レイヤー: メインの約50%と100%
- 3レイヤー: メインの約33%、約67%、100%
- 4レイヤー: メインの25%、50%、75%、100%

このPluginでは、その固定的な分け方に縛られません。Layer 1はOBSのメイン設定をそのまま使用しながら、
Layer 2～4の**解像度・FPS・ビットレートを、それぞれ狙った値で明示指定**できます。

例えば、メインが`1920x1080 / 60 fps / 8000 kbps`でも、サブレイヤーを単純な50%や33%にせず、
用途に合わせて`1280x720 / 60 fps / 4500 kbps`、`854x480 / 30 fps / 1200 kbps`のように設計できます。

OBS本体、`obs-webrtc`、SDP、WebRTC Transportは変更しません。Encoder制御には公開Frontend API / libobs APIを使用し、
Settingsへの表示部分だけOBSのQt widget treeへ安全に追加します。

> [!IMPORTANT]
> 現在はWindows x64向けのベータ版です。OBS Studio 32.2.1を対象にビルドしています。
> 32.2.1より前のOBSはサポートせず、将来のOBSでは設定画面の変更によりToolsメニューのfallback画面が必要になる場合があります。
> 初期ReleaseのSetupとDLLにはコード署名がないため、Windowsに「不明な発行元」と表示されます。

このプロジェクトはコミュニティ製であり、OBS Project公式のプラグインではありません。

## ダウンロード

[GitHub Releases](https://github.com/yuyutti/WHIP-Custom-Simulcast/releases)から、
`whip-custom-simulcast-<version>-windows-x64-setup.exe`をダウンロードしてください。

手動配置用として`whip-custom-simulcast-<version>-windows-x64.zip`も配布します。

## 機能

- Windows x64 / OBS Studio 32.2.1
- WHIP Outputだけを対象にする
- OBS標準の1～4 Total Layersと同期
- Main Layer（Encoder index 0）は常にOBS設定を維持
- Layer 2～4の個別設定
  - 解像度
  - フレームレート
  - ビットレート
- Plugin ON/OFF
- Settings > Stream > WHIP > Simulcast内への設定UI追加
- Apply / OKで保存、Cancelで破棄
- OBS module configの独立JSONへ安全に保存
- Tools > WHIP Custom Simulcastのfallback設定画面
- Encoder適用失敗時のレイヤー単位ロールバック
- 日本語 / 英語UI

## 動作

`OBS_FRONTEND_EVENT_STREAMING_STARTING` で現在のStreaming Outputを取得し、次の条件を満たす場合だけ設定を適用します。

```text
Plugin enabled
AND Output ID == whip_output
AND WHIP Total Layers >= 2
AND 対象レイヤーの設定が有効
AND 対象Encoderが存在し、まだactiveではない
```

Encoder indexとWHIP RIDの対応は次のとおりです。

| Encoder index | WHIP RID | 動作 |
|---|---:|---|
| 0 | 0 | OBSのMain設定を維持 |
| 1 | 1 | Layer 2設定を適用 |
| 2 | 2 | Layer 3設定を適用 |
| 3 | 3 | Layer 4設定を適用 |

## 初期値

初回起動時はPluginが無効です。設定値は次の状態で準備されます。

| Layer | 解像度 | FPS | Bitrate |
|---|---:|---:|---:|
| 1 | OBS Main | OBS Main | OBS Main |
| 2 | 854x480 | 30 | 700 kbps |
| 3 | 640x360 | 30 | 500 kbps |
| 4 | 426x240 | 15 | 250 kbps |

## 設定UI

1. OBSで「設定 > 配信」を開く。
2. ServiceにWHIPを選択する。
3. Simulcastの「合計レイヤー数」を1～4から選択する。
4. `WHIP Custom Simulcast` でカスタム設定を有効にする。
5. 1行に1レイヤーで表示されたLayer 2～4を設定する。
6. ApplyまたはOKを押す。

Layer 1は現在のOBS出力解像度、FPS、配信bitrateを表示し、編集できない状態になります。Layer 2以降は編集でき、
「合計レイヤー数」を超える行は表示されません。

Cancelまたはウィンドウの閉じる操作では、未保存のPlugin設定を破棄します。OBS更新によりSettingsへのUI追加に失敗した場合でも、
「Tools > WHIP Custom Simulcast」から同じ設定を編集できます。fallback画面ではTotal Layersは変更せず、現在のOBS Profile値を使用します。

## 入力検証

- 幅 / 高さ: 32以上、現在のOBS Output Resolution以下
- 幅 / 高さ: 偶数
- Bitrate: 50～100000 kbps
- FPS: OBSの公称基本FPSを整数で割り切れる値

60 fpsの場合は60 / 30 / 20 / 15 / 12 / 10などを選択できます。59.94 fpsや29.97 fpsは、それぞれ公称60 fps / 30 fpsとして
divisorを計算するため、30 fps指定時の実際の送信FPSは29.97 fpsになります。

## 設定ファイル

設定はOBSのmodule configディレクトリに保存されます。

```text
plugin_config/whip-custom-simulcast/config.json
```

概念例:

```json
{
    "version": 1,
    "enabled": true,
    "layers": [
        {
            "use_obs_main": true
        },
        {
            "width": 854,
            "height": 480,
            "fps": 30,
            "bitrate_kbps": 700
        }
    ]
}
```

実際のTotal LayersはOBS Profileの `Stream1.WHIPSimulcastTotalLayers` をSource of Truthとします。Plugin JSONには最大4 Layer分を保持するため、
Total Layersを減らした後に増やしても以前の値を復元できます。保存時には一時ファイルとバックアップを使用します。

## ビルド

このリポジトリは公式OBS Plugin Templateのビルド構成を使用します。初回configure時にOBS 32.2.1のソースと依存関係を `.deps` へ取得します。

必要なもの:

- Visual Studio 2022（Desktop development with C++）
- Windows 10/11 SDK
- CMake 3.28～3.30

PowerShell:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Visual Studio付属CMakeがPATHにない場合:

```powershell
$cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --preset windows-x64
& $cmake --build --preset windows-x64
```

成果物:

```text
build_x64/RelWithDebInfo/whip-custom-simulcast.dll
```

## インストール

1. OBSを終了する。
2. ReleaseからダウンロードしたSetupを実行する。
3. 「現在のユーザーのみ」または「すべてのユーザー」を選択する。
4. インストールを完了してOBSを起動する。

「現在のユーザーのみ」は管理者権限なしで、そのWindowsユーザーだけにインストールします。「すべてのユーザー」は
管理者権限を要求し、PC上の全ユーザーが利用できる`C:\ProgramData\obs-studio\plugins`へインストールします。

> [!NOTE]
> OBS 32.2.1のWindows版は、ユーザー別Pluginフォルダーを標準では探索しません。そのため「現在のユーザーのみ」では、
> ユーザー環境変数`OBS_PLUGINS_PATH`と`OBS_PLUGINS_DATA_PATH`を設定します。既存の値と競合する場合、Setupは上書きせず停止します。

同じSetupをインストール後にもう一度実行すると、次の保守操作を選択できます。

- 修復: DLL、locale、ドキュメントを再配置する
- アンインストール: Plugin本体を削除する

Pluginの設定値はアンインストールしても残します。

### ZIPによる手動インストール

ZIP内の`whip-custom-simulcast`フォルダーを`C:\ProgramData\obs-studio\plugins`へコピーします。
この方法では全ユーザー向けの配置になります。

配置後は次の構成になります。

```text
C:\ProgramData\obs-studio\plugins\whip-custom-simulcast\
├── bin\64bit\whip-custom-simulcast.dll
├── data\locale\
│   ├── en-US.ini
│   └── ja-JP.ini
├── CHANGELOG.md
├── LICENSE
├── README.en.md
└── README.md
```

localeファイルを配置できていない場合も、画面にキー名を表示せず、OBSの言語に応じた内蔵の日本語または英語へfallbackします。

### 更新・アンインストール

- 更新: OBSを終了し、新しいSetupを実行します。
- 修復・アンインストール: OBSを終了し、インストールに使用したSetupをもう一度実行します。
- Windowsの「インストールされているアプリ」からアンインストールすることもできます。
- Plugin設定を初期化する場合だけ、`%APPDATA%\obs-studio\plugin_config\whip-custom-simulcast`も削除します。

## 検証状況

Releaseビルド、設定値の自動テスト、インストーラーとZIPの構成検査は自動化しています。
利用者が開発用テストを行う必要はありません。

ただし、接続先のWHIP/SFUごとの相互運用性や実際の受信品質まで自動テストだけで保証することはできないため、
現在はベータ版として公開します。不具合が発生した場合はOBSログを添えてIssueへ報告してください。

## 現在の対象外

- 独自WHIP / WHEP実装
- SDP / RTP操作
- Main Layerのカスタム化
- レイヤー別Codec / Encoder preset
- 配信中の動的な解像度・FPS変更
- 5～10 Layers
- macOS / Linux正式対応

## バグ報告・機能要望

[GitHub Issues](https://github.com/yuyutti/WHIP-Custom-Simulcast/issues)を利用してください。バグ報告にはOBSバージョン、
Pluginバージョン、Encoder、再現手順、OBSログを含めてください。ログを添付する前にWHIP URL、Bearer Token、配信キーなどを削除してください。

開発参加については[CONTRIBUTING.md](CONTRIBUTING.md)、脆弱性の報告方法は[SECURITY.md](SECURITY.md)、
リリース履歴は[CHANGELOG.md](CHANGELOG.md)を参照してください。

## ライセンス

GPL-2.0-or-later。詳細は[LICENSE](LICENSE)を参照してください。
