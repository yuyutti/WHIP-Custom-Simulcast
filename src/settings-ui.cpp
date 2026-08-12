// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#include "settings-ui.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/config-file.h>

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <array>
#include <cstring>

#define LOG_PREFIX "[WHIP Custom Simulcast] "

namespace whip_custom_simulcast {
namespace {

constexpr int kSettingsSectionSpacing = 8;

struct FallbackText {
    const char *key;
    const char *english;
    const char *japanese;
};

constexpr std::array kFallbackTexts = {
    FallbackText{"WHIPCustomSimulcast.Name", "WHIP Custom Simulcast", "WHIP Custom Simulcast"},
    FallbackText{"Settings.Title", "WHIP Custom Simulcast", "WHIP Custom Simulcast"},
    FallbackText{"Settings.Enable", "Enable custom layer configuration", "カスタムレイヤー設定を有効にする"},
    FallbackText{"Settings.MainLayerTooltip", "Layer 1 uses the current OBS output settings.",
                 "レイヤー1は現在のOBS出力設定を使用します。"},
    FallbackText{"Settings.UnsupportedFps", "%1 fps (unsupported by current OBS FPS)",
                 "%1 fps（現在のOBS FPSでは使用不可）"},
    FallbackText{"Settings.DisabledStatus", "Custom layer configuration is disabled. OBS defaults will be used.",
                 "カスタムレイヤー設定は無効です。OBS標準設定を使用します。"},
    FallbackText{"Settings.OneLayerStatus", "Total Layers is 1. No custom layer will be applied.",
                 "合計レイヤー数が1のため、カスタムレイヤーは適用されません。"},
    FallbackText{"Settings.ValidStatus", "The active custom layer settings are valid.",
                 "使用中のカスタムレイヤー設定は有効です。"},
    FallbackText{"Settings.InvalidStatus", "Correct the highlighted settings before saving.",
                 "保存する前に、エラーが表示された設定を修正してください。"},
    FallbackText{"Settings.LayerError", "Layer %1: %2", "レイヤー%1: %2"},
    FallbackText{"Validation.MinimumResolution", "Resolution must be at least 32x32.",
                 "解像度は32x32以上にしてください。"},
    FallbackText{"Validation.EvenDimensions", "Width and height must be even numbers.",
                 "幅と高さは偶数にしてください。"},
    FallbackText{"Validation.ExceedsOutputResolution", "Resolution exceeds the current OBS output resolution.",
                 "解像度が現在のOBS出力解像度を超えています。"},
    FallbackText{"Validation.BitrateRange", "Bitrate must be between 50 and 100000 kbps.",
                 "ビットレートは50～100000 kbpsにしてください。"},
    FallbackText{"Validation.UnsupportedBaseFps",
                 "The current OBS base FPS is not supported. Use a standard integer or NTSC-derived frame rate up "
                 "to 240 fps.",
                 "現在のOBS基本FPSには対応していません。240 "
                 "fps以下の標準的な整数またはNTSC系フレームレートを使用してください。"},
    FallbackText{"Validation.UnsupportedTargetFps", "FPS must be an integer divisor of the nominal OBS base FPS.",
                 "FPSはOBSの公称基本FPSを整数で割り切れる値にしてください。"},
    FallbackText{"Validation.VideoUnavailable", "OBS video information is unavailable.",
                 "OBSの映像情報を取得できません。"},
    FallbackText{"Validation.Unknown", "Unknown validation error.", "不明な設定エラーです。"},
    FallbackText{"Dialog.Title", "WHIP Custom Simulcast", "WHIP Custom Simulcast"},
    FallbackText{"Dialog.SaveFailed", "The configuration could not be saved. Check the OBS log.",
                 "設定を保存できませんでした。OBSログを確認してください。"},
    FallbackText{"Dialog.InvalidSettings", "Correct the invalid custom layer settings before saving.",
                 "無効なカスタムレイヤー設定を修正してから保存してください。"},
    FallbackText{"Dialog.LayerCount", "OBS WHIP Total Layers: %1. Change this value in Settings > Stream.",
                 "OBS WHIPの合計レイヤー数: %1。この値は「設定 > 配信」で変更してください。"},
    FallbackText{"Tools.Menu", "WHIP Custom Simulcast", "WHIP Custom Simulcast"},
};

bool useJapaneseFallback() {
    const char *locale = obs_get_locale();
    return locale && std::strncmp(locale, "ja", 2) == 0;
}

QString moduleText(const char *key) {
    const char *translated = nullptr;
    if (obs_module_get_string(key, &translated) && translated && std::strcmp(translated, key) != 0) {
        return QString::fromUtf8(translated);
    }

    static bool fallbackLogged = false;
    if (!fallbackLogged) {
        blog(LOG_WARNING, LOG_PREFIX "Locale resources unavailable; using built-in fallback strings");
        fallbackLogged = true;
    }

    const bool japanese = useJapaneseFallback();
    const auto match = std::find_if(kFallbackTexts.begin(), kFallbackTexts.end(),
                                    [key](const FallbackText &text) { return std::strcmp(text.key, key) == 0; });
    if (match != kFallbackTexts.end()) {
        return QString::fromUtf8(japanese ? match->japanese : match->english);
    }

    return QString::fromUtf8(key);
}

QString validationText(LayerValidationError error) {
    switch (error) {
    case LayerValidationError::None:
        return {};
    case LayerValidationError::MinimumResolution:
        return moduleText("Validation.MinimumResolution");
    case LayerValidationError::EvenDimensions:
        return moduleText("Validation.EvenDimensions");
    case LayerValidationError::ExceedsOutputResolution:
        return moduleText("Validation.ExceedsOutputResolution");
    case LayerValidationError::BitrateRange:
        return moduleText("Validation.BitrateRange");
    case LayerValidationError::UnsupportedBaseFps:
        return moduleText("Validation.UnsupportedBaseFps");
    case LayerValidationError::UnsupportedTargetFps:
        return moduleText("Validation.UnsupportedTargetFps");
    }

    return moduleText("Validation.Unknown");
}

} // namespace

SettingsPanel::SettingsPanel(ConfigManager &configManager, int layerCount, QWidget *parent)
    : QWidget(parent), configManager_(configManager), savedConfig_(configManager.current()) {
    setObjectName("whipCustomSimulcastSettingsPanel");
    buildUi();
    populateFromConfig();
    setLayerCount(layerCount);
}

void SettingsPanel::buildUi() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    auto *title = new QLabel(moduleText("Settings.Title"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);
    layout->addSpacing(kSettingsSectionSpacing);

    enabled_ = new QCheckBox(moduleText("Settings.Enable"), this);
    layout->addWidget(enabled_);

    obs_video_info videoInfo{};
    obs_get_video_info(&videoInfo);

    for (size_t index = 0; index < kMaxLayers; ++index) {
        LayerEditor &editor = editors_[index];
        editor.row = new QWidget(this);
        editor.row->setObjectName(QStringLiteral("whipCustomSimulcastLayer%1").arg(index + 1));
        auto *rowLayout = new QHBoxLayout(editor.row);
        rowLayout->setContentsMargins(4, 0, 4, 0);
        rowLayout->setSpacing(6);

        auto *number = new QLabel(QString::number(index + 1), editor.row);
        number->setAlignment(Qt::AlignCenter);
        number->setMinimumWidth(18);
        rowLayout->addWidget(number);

        editor.width = new QSpinBox(editor.row);
        editor.height = new QSpinBox(editor.row);
        for (QSpinBox *spinBox : {editor.width, editor.height}) {
            spinBox->setRange(index == 0 ? 0 : static_cast<int>(kMinimumDimension),
                              static_cast<int>(kMaximumDimension));
            spinBox->setSingleStep(2);
            spinBox->setMinimumWidth(95);
            if (index == 0) {
                spinBox->setSpecialValueText(QStringLiteral("—"));
            }
        }
        rowLayout->addWidget(editor.width, 1);
        rowLayout->addWidget(new QLabel(QStringLiteral("×"), editor.row));
        rowLayout->addWidget(editor.height, 1);
        rowLayout->addWidget(new QLabel(QStringLiteral("px"), editor.row));

        editor.fps = new QComboBox(editor.row);
        editor.fps->setMinimumWidth(80);
        const uint32_t selectedFps = index == 0 ? nominalBaseFps(videoInfo) : savedConfig_.layers[index].fps;
        populateFpsOptions(editor, selectedFps);
        rowLayout->addWidget(editor.fps, 1);
        rowLayout->addWidget(new QLabel(QStringLiteral("fps"), editor.row));

        editor.bitrate = new QSpinBox(editor.row);
        editor.bitrate->setRange(index == 0 ? 0 : static_cast<int>(kMinimumBitrateKbps),
                                 static_cast<int>(kMaximumBitrateKbps));
        editor.bitrate->setMinimumWidth(105);
        if (index == 0) {
            editor.bitrate->setSpecialValueText(QStringLiteral("—"));
        }
        rowLayout->addWidget(editor.bitrate, 1);
        rowLayout->addWidget(new QLabel(QStringLiteral("kbps"), editor.row));

        layout->addWidget(editor.row);

        if (index == 0) {
            editor.row->setEnabled(false);
            editor.row->setToolTip(moduleText("Settings.MainLayerTooltip"));
            continue;
        }

        connect(editor.width, &QSpinBox::valueChanged, this, [this]() { handleEditorChanged(); });
        connect(editor.height, &QSpinBox::valueChanged, this, [this]() { handleEditorChanged(); });
        connect(editor.fps, &QComboBox::currentIndexChanged, this, [this]() { handleEditorChanged(); });
        connect(editor.bitrate, &QSpinBox::valueChanged, this, [this]() { handleEditorChanged(); });
    }

    layout->addSpacing(kSettingsSectionSpacing);
    status_ = new QLabel(this);
    status_->setWordWrap(true);
    layout->addWidget(status_);

    connect(enabled_, &QCheckBox::toggled, this, [this]() { handleEditorChanged(); });
}

void SettingsPanel::populateFromConfig() {
    enabled_->setChecked(savedConfig_.enabled);
    refreshMainLayer();

    for (size_t index = 1; index < kMaxLayers; ++index) {
        LayerEditor &editor = editors_[index];
        const LayerConfig &layer = savedConfig_.layers[index];
        editor.width->setValue(static_cast<int>(layer.width));
        editor.height->setValue(static_cast<int>(layer.height));
        editor.bitrate->setValue(static_cast<int>(layer.bitrateKbps));

        const int fpsIndex = editor.fps->findData(layer.fps);
        if (fpsIndex >= 0) {
            editor.fps->setCurrentIndex(fpsIndex);
        }
    }
}

void SettingsPanel::refreshMainLayer() {
    LayerEditor &editor = editors_[0];
    obs_video_info videoInfo{};

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t fps = 0;
    uint32_t bitrate = 0;
    if (obs_get_video_info(&videoInfo)) {
        width = videoInfo.output_width;
        height = videoInfo.output_height;
        fps = nominalBaseFps(videoInfo);
    }

    obs_output_t *output = obs_frontend_get_streaming_output();
    if (output) {
        obs_encoder_t *encoder = obs_output_get_video_encoder2(output, 0);
        if (encoder) {
            const uint32_t encoderWidth = obs_encoder_get_width(encoder);
            const uint32_t encoderHeight = obs_encoder_get_height(encoder);
            const uint32_t divisor = obs_encoder_get_frame_rate_divisor(encoder);
            width = encoderWidth > 0 ? encoderWidth : width;
            height = encoderHeight > 0 ? encoderHeight : height;
            if (fps > 0 && divisor > 0) {
                fps /= divisor;
            }

            obs_data_t *settings = obs_encoder_get_settings(encoder);
            if (settings) {
                bitrate = static_cast<uint32_t>(std::max<int64_t>(0, obs_data_get_int(settings, "bitrate")));
                obs_data_release(settings);
            }
        }
        obs_output_release(output);
    }

    if (bitrate == 0) {
        config_t *profile = obs_frontend_get_profile_config();
        const char *outputMode = profile ? config_get_string(profile, "Output", "Mode") : nullptr;
        if (outputMode && std::strcmp(outputMode, "Simple") == 0) {
            bitrate = static_cast<uint32_t>(std::max<int64_t>(0, config_get_int(profile, "SimpleOutput", "VBitrate")));
        }
    }

    const QSignalBlocker widthBlocker(editor.width);
    const QSignalBlocker heightBlocker(editor.height);
    const QSignalBlocker fpsBlocker(editor.fps);
    const QSignalBlocker bitrateBlocker(editor.bitrate);
    editor.width->setValue(static_cast<int>(width));
    editor.height->setValue(static_cast<int>(height));
    editor.bitrate->setValue(static_cast<int>(bitrate));

    if (fps == 0) {
        editor.fps->clear();
        editor.fps->addItem(QStringLiteral("—"), fps);
    } else {
        const int fpsIndex = editor.fps->findData(fps);
        if (fpsIndex >= 0) {
            editor.fps->setCurrentIndex(fpsIndex);
        } else {
            editor.fps->clear();
            editor.fps->addItem(QString::number(fps), fps);
        }
    }
}

void SettingsPanel::populateFpsOptions(LayerEditor &editor, uint32_t selectedFps) {
    obs_video_info videoInfo{};
    std::vector<uint32_t> values;
    if (obs_get_video_info(&videoInfo)) {
        values = supportedTargetFps(videoInfo);
    }

    for (uint32_t fps : values) {
        editor.fps->addItem(QString::number(fps), fps);
    }

    if (std::find(values.begin(), values.end(), selectedFps) == values.end()) {
        editor.fps->addItem(moduleText("Settings.UnsupportedFps").arg(selectedFps), selectedFps);
    }
}

void SettingsPanel::setLayerCount(int layerCount) {
    layerCount_ = std::clamp(layerCount, 1, static_cast<int>(kMaxLayers));
    refreshMainLayer();
    for (size_t index = 0; index < kMaxLayers; ++index) {
        editors_[index].row->setVisible(static_cast<int>(index) < layerCount_);
    }
    validate();
}

void SettingsPanel::attachButtons(QPushButton *applyButton, QPushButton *okButton) {
    applyButton_ = applyButton;
    okButton_ = okButton;
    initialApplyEnabled_ = applyButton ? applyButton->isEnabled() : false;

    if (applyButton) {
        applyButton->installEventFilter(this);
    }
    if (okButton) {
        okButton->installEventFilter(this);
    }

    updateButtons();
}

PluginConfig SettingsPanel::collectConfig() const {
    PluginConfig config = savedConfig_;
    config.enabled = enabled_->isChecked();

    for (size_t index = 1; index < kMaxLayers; ++index) {
        const LayerEditor &editor = editors_[index];
        config.layers[index] = {
            static_cast<uint32_t>(editor.width->value()),
            static_cast<uint32_t>(editor.height->value()),
            editor.fps->currentData().toUInt(),
            static_cast<uint32_t>(editor.bitrate->value()),
        };
    }

    return config;
}

void SettingsPanel::handleEditorChanged() {
    dirty_ = !(collectConfig() == savedConfig_);
    validate();
}

void SettingsPanel::validate() {
    valid_ = true;
    obs_video_info videoInfo{};
    const bool haveVideoInfo = obs_get_video_info(&videoInfo);
    const PluginConfig config = collectConfig();
    QStringList errors;

    for (size_t index = 1; index < kMaxLayers; ++index) {
        LayerEditor &editor = editors_[index];
        editor.row->setStyleSheet(QString());
        editor.row->setToolTip(QString());

        if (!config.enabled || static_cast<int>(index) >= layerCount_) {
            continue;
        }

        QString error;
        if (!haveVideoInfo) {
            error = moduleText("Validation.VideoUnavailable");
        } else {
            const LayerValidation result = validateLayerConfig(config.layers[index], videoInfo);
            if (!result.valid) {
                error = validationText(result.error);
            }
        }

        if (!error.isEmpty()) {
            valid_ = false;
            const QString layerError = moduleText("Settings.LayerError").arg(index + 1).arg(error);
            errors.push_back(layerError);
            editor.row->setToolTip(error);
            editor.row->setStyleSheet(
                QStringLiteral("QSpinBox, QComboBox { border: 1px solid #d9534f; border-radius: 2px; }"));
        }
    }

    if (!config.enabled) {
        status_->setText(moduleText("Settings.DisabledStatus"));
        status_->setStyleSheet(QString());
    } else if (layerCount_ < 2) {
        status_->setText(moduleText("Settings.OneLayerStatus"));
        status_->setStyleSheet(QString());
    } else if (valid_) {
        status_->setText(moduleText("Settings.ValidStatus"));
        status_->setStyleSheet(QStringLiteral("color: #3c9a5f;"));
    } else {
        status_->setText(errors.join(QLatin1Char('\n')));
        status_->setStyleSheet(QStringLiteral("color: #d9534f;"));
    }

    updateButtons();
}

void SettingsPanel::updateButtons() {
    if (okButton_) {
        okButton_->setEnabled(valid_);
    }

    if (applyButton_) {
        const bool obsApplyEnabled = applyButton_->isEnabled();
        applyButton_->setEnabled(valid_ && (dirty_ || initialApplyEnabled_ || obsApplyEnabled));
    }
}

bool SettingsPanel::commit() {
    validate();
    if (!valid_) {
        return false;
    }

    const PluginConfig config = collectConfig();
    if (!configManager_.save(config)) {
        QMessageBox::warning(this, moduleText("Dialog.Title"), moduleText("Dialog.SaveFailed"));
        return false;
    }

    savedConfig_ = config;
    dirty_ = false;
    initialApplyEnabled_ = false;
    updateButtons();
    return true;
}

bool SettingsPanel::eventFilter(QObject *watched, QEvent *event) {
    if (!valid_ && (watched == applyButton_ || watched == okButton_) &&
        (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::KeyPress)) {
        QMessageBox::warning(this, moduleText("Dialog.Title"), moduleText("Dialog.InvalidSettings"));
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

SettingsUiInjector::SettingsUiInjector(ConfigManager &configManager) : configManager_(configManager) {}

SettingsUiInjector::~SettingsUiInjector() { uninstall(); }

void SettingsUiInjector::install() {
    if (installed_ || !qApp) {
        return;
    }

    qApp->installEventFilter(this);
    const QByteArray toolsMenuText = moduleText("Tools.Menu").toUtf8();
    toolsAction_ = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(toolsMenuText.constData()));
    if (toolsAction_) {
        connect(toolsAction_, &QAction::triggered, this, [this]() { openFallbackDialog(); });
    }

    installed_ = true;
    blog(LOG_INFO, LOG_PREFIX "Settings UI event filter installed");
}

void SettingsUiInjector::uninstall() {
    if (!installed_) {
        return;
    }

    if (qApp) {
        qApp->removeEventFilter(this);
    }

    if (fallbackDialog_) {
        delete fallbackDialog_.data();
    }

    for (const QPointer<SettingsPanel> &panel : injectedPanels_) {
        if (panel) {
            delete panel.data();
        }
    }
    injectedPanels_.clear();

    if (toolsAction_) {
        delete toolsAction_;
    }

    installed_ = false;
}

bool SettingsUiInjector::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Show) {
        auto *dialog = qobject_cast<QDialog *>(watched);
        if (dialog && dialog->objectName() == QStringLiteral("OBSBasicSettings")) {
            scheduleInjection(dialog);
        }
    }

    return QObject::eventFilter(watched, event);
}

void SettingsUiInjector::scheduleInjection(QDialog *dialog) {
    const QPointer<QDialog> guardedDialog(dialog);
    QTimer::singleShot(0, this, [this, guardedDialog]() {
        if (guardedDialog) {
            inject(guardedDialog);
        }
    });
}

void SettingsUiInjector::inject(QDialog *dialog) {
    if (dialog->findChild<QWidget *>(QStringLiteral("whipCustomSimulcastSettingsPanel"))) {
        return;
    }

    auto *group = dialog->findChild<QGroupBox *>(QStringLiteral("whipSimulcastGroupBox"));
    auto *layerCount = dialog->findChild<QSpinBox *>(QStringLiteral("whipSimulcastTotalLayers"));
    auto *buttonBox = dialog->findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));

    if (!group || !group->layout() || !layerCount || !buttonBox) {
        blog(LOG_WARNING, LOG_PREFIX "Failed to locate OBS WHIP Settings widgets; use the Tools menu fallback");
        return;
    }

    auto *panel = new SettingsPanel(configManager_, layerCount->value(), group);
    group->layout()->addWidget(panel);
    injectedPanels_.push_back(panel);

    QPushButton *applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    panel->attachButtons(applyButton, okButton);

    connect(layerCount, &QSpinBox::valueChanged, panel, [panel](int value) { panel->setLayerCount(value); });
    connect(buttonBox, &QDialogButtonBox::clicked, panel, [panel, buttonBox](QAbstractButton *button) {
        const auto role = buttonBox->buttonRole(button);
        if (role == QDialogButtonBox::ApplyRole || role == QDialogButtonBox::AcceptRole) {
            panel->commit();
        }
    });

    blog(LOG_INFO, LOG_PREFIX "WHIP Settings UI injected");
}

int SettingsUiInjector::currentLayerCount() const {
    config_t *profile = obs_frontend_get_profile_config();
    if (!profile) {
        return 1;
    }

    return std::clamp(static_cast<int>(config_get_int(profile, "Stream1", "WHIPSimulcastTotalLayers")), 1,
                      static_cast<int>(kMaxLayers));
}

void SettingsUiInjector::openFallbackDialog() {
    if (fallbackDialog_) {
        fallbackDialog_->show();
        fallbackDialog_->raise();
        fallbackDialog_->activateWindow();
        return;
    }

    auto *parent = static_cast<QWidget *>(obs_frontend_get_main_window());
    auto *dialog = new QDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(moduleText("Dialog.Title"));
    dialog->resize(760, 300);
    fallbackDialog_ = dialog;

    auto *layout = new QVBoxLayout(dialog);
    const int layerCount = currentLayerCount();
    auto *layerCountInfo = new QLabel(moduleText("Dialog.LayerCount").arg(layerCount), dialog);
    layerCountInfo->setWordWrap(true);
    layout->addWidget(layerCountInfo);

    auto *panel = new SettingsPanel(configManager_, layerCount, dialog);
    layout->addWidget(panel);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, dialog);
    layout->addWidget(buttonBox);
    QPushButton *saveButton = buttonBox->button(QDialogButtonBox::Save);
    panel->attachButtons(nullptr, saveButton);

    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(saveButton, &QPushButton::clicked, dialog, [panel, dialog]() {
        if (panel->commit()) {
            dialog->accept();
        }
    });

    dialog->show();
}

} // namespace whip_custom_simulcast
