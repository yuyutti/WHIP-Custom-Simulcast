# Release手順

## 1. Release候補を準備する

1. `buildspec.json`の`version`をSemantic Versioning形式で更新する。
2. `CHANGELOG.md`へ変更点とRelease日を記載する。
3. `README.md`と`README.en.md`の対応環境、設定、制限事項を同期する。
4. `data/locale/en-US.ini`と`data/locale/ja-JP.ini`のキーが一致することを確認する。

## 2. ローカル検証

```powershell
cmake --preset windows-ci-x64
cmake --build build_x64 --config Release --parallel
ctest --test-dir build_x64 -C Release --output-on-failure
cmake --install build_x64 --config Release --prefix release/Release
```

`release/Release/whip-custom-simulcast`にDLL、日英locale、LICENSE、README、CHANGELOGが含まれることを確認します。
続いてInno Setup 6で`installer/WHIP-Custom-Simulcast.iss`をコンパイルし、Setupを生成します。

続いてクリーンなOBS Studio 32.2.1環境で、次を確認します。

- Pluginがロードされる
- 日本語と英語の表示でキー名が露出しない
- Total Layersに応じて表示行数が変化する
- Layer 1がOBS値を表示し、編集できない
- Apply、OK、Cancelが正しく動作する
- 実際のWHIP配信でRID、解像度、FPS、bitrateが期待どおりになる
- Pluginを無効にするとOBS標準動作へ戻る
- Setupで現在ユーザー／全ユーザーを選択できる
- Setupの再実行で修復／アンインストールを選択できる

## 3. GitHub Releaseを作成する

Release tagには`v`を付けず、Stable Releaseでは`buildspec.json`と完全に同じバージョンを使用します。
Prereleaseでは、`buildspec.json`の数値部分に`-beta1`または`-rc1`のような接尾辞を付けます。

```powershell
git tag -a 0.1.0 -m "WHIP Custom Simulcast 0.1.0"
git push origin 0.1.0
```

GitHub ActionsはReleaseビルドとCTestを実行し、SetupとZIPの必須ファイルを検査します。その後、
Windows x64用SetupとZIPを添付したDraft ReleaseをGitHub上に作成します。

## 4. Draftを公開する

1. Windows x64用SetupとZIPの2ファイルだけが添付されていることを確認する。
2. SetupとZIPをダウンロードし、クリーン環境へインストールする。
3. 自動生成されたRelease Notesと`CHANGELOG.md`を照合する。
4. 署名していないバイナリであることをRelease Notesに明記する。
5. DraftをPublishする。

## 5. OBS Forumへ掲載する

`docs/OBS_FORUM_RESOURCE.md`を元にResourceページを作成し、GitHub Release、Source、Issue Tracker、Licenseへの
リンクを登録します。機能や対応OSを変更したReleaseではForum側の説明も更新します。

GitHubへのpush、Draftの公開、OBS Forumへの投稿は外部公開操作なので、必ず内容を目視確認してから実行してください。
