// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 yuyutti

#pragma once

#include "plugin-config.hpp"

#include <QObject>
#include <QPointer>
#include <QWidget>

#include <array>
#include <vector>

class QAction;
class QCheckBox;
class QComboBox;
class QDialog;
class QDialogButtonBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace whip_custom_simulcast {

class SettingsPanel : public QWidget {
  public:
    SettingsPanel(ConfigManager &configManager, int layerCount, QWidget *parent = nullptr);

    void setLayerCount(int layerCount);
    void attachButtons(QPushButton *applyButton, QPushButton *okButton);
    void refreshObsSettings();
    bool commit();

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    struct LayerEditor {
        QWidget *row = nullptr;
        QSpinBox *width = nullptr;
        QSpinBox *height = nullptr;
        QComboBox *fps = nullptr;
        QSpinBox *bitrate = nullptr;
    };

    void buildUi();
    void populateFromConfig();
    void refreshMainLayer();
    void populateFpsOptions(LayerEditor &editor, uint32_t selectedFps);
    PluginConfig collectConfig() const;
    void handleEditorChanged();
    void validate();
    void updateButtons();

    ConfigManager &configManager_;
    PluginConfig savedConfig_;
    int layerCount_ = 1;
    bool valid_ = true;
    bool dirty_ = false;
    bool initialApplyEnabled_ = false;
    QCheckBox *enabled_ = nullptr;
    QLabel *status_ = nullptr;
    std::array<LayerEditor, kMaxLayers> editors_{};
    QPointer<QPushButton> applyButton_;
    QPointer<QPushButton> okButton_;
};

class SettingsUiInjector : public QObject {
  public:
    explicit SettingsUiInjector(ConfigManager &configManager);
    ~SettingsUiInjector() override;

    void install();
    void uninstall();
    void openFallbackDialog();

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void scheduleInjection(QDialog *dialog);
    void inject(QDialog *dialog);
    int currentLayerCount() const;

    ConfigManager &configManager_;
    bool installed_ = false;
    QPointer<QAction> toolsAction_;
    QPointer<QDialog> fallbackDialog_;
    std::vector<QPointer<SettingsPanel>> injectedPanels_;
};

} // namespace whip_custom_simulcast
