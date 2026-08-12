# Contributing

WHIP Custom Simulcastへのバグ報告、改善提案、Pull Requestを歓迎します。

## Issueを作成する前に

- 最新のReleaseとOBS Studio 32.2.1で再現するか確認してください。
- 同じ内容のIssueがないか検索してください。
- OBSログを共有する場合は、WHIP URL、Bearer Token、配信キー、ユーザー名などの秘密情報を削除してください。

## 開発環境

- Windows 10/11 x64
- Visual Studio 2022（Desktop development with C++）
- Windows 10/11 SDK
- CMake 3.28～3.30
- Inno Setup 6（インストーラーを生成する場合）
- OBS Studio 32.2.1

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
```

初回configureではOBSとビルド依存関係を`.deps`へダウンロードします。

## Pull Request

1. 変更範囲を小さく保ち、目的を説明してください。
2. 動作を変更する場合はテストと日英ドキュメントを更新してください。
3. ユーザー向け文言を追加する場合は`data/locale/en-US.ini`と`data/locale/ja-JP.ini`を同時に更新してください。
4. 実際のWHIP配信を確認していない場合は、ビルド確認と実機確認を明確に区別してください。
5. `clang-format`、CMake formatting、Releaseビルド、CTestが成功する状態にしてください。

Pull Requestとして提供されたコードは、このリポジトリと同じGPL-2.0-or-laterで配布されます。

## 実機確認で添える情報

- OBSとPluginのバージョン
- Windowsバージョン
- Encoder名と出力モード
- WHIP Simulcast Total Layers
- 各レイヤーの設定値
- OBSログの関連部分
- 可能であればSFU側のRID、WebRTC stats、実測帯域

秘密情報や第三者の個人情報は含めないでください。
