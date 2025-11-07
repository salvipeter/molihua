#pragma once

#include <optional>

#include <QDialog>

#include <libcdgbs/Mesh.hpp>

class DomainWindow : public QDialog {
  Q_OBJECT
public:
  DomainWindow(QWidget *parent);
  void setDomain(const std::optional<libcdgbs::Mesh> &mesh);
private:
  void closeEvent(QCloseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void showEvent(QShowEvent *event) override;

  libcdgbs::Mesh domain;
};
