#include "tababfuellen.h"
#include "ui_tababfuellen.h"
#include <QMessageBox>
#include <qmath.h>
#include "brauhelfer.h"
#include "settings.h"
#include "templatetags.h"
#include "dialogs/dlgrestextrakt.h"

extern Brauhelfer* bh;
extern Settings* gSettings;

TabAbfuellen::TabAbfuellen(QWidget *parent) :
    TabAbstract(parent),
    ui(new Ui::TabAbfuellen)
{
    ui->setupUi(this);
    ui->tbReifezeit->setColumn(ModelSud::ColWoche);
    ui->tbSWSchnellgaerprobe->setColumn(ModelSud::ColSWSchnellgaerprobe);
    ui->tbSWJungbier->setColumn(ModelSud::ColSWJungbier);
    ui->tbBiermengeAbfuellen->setColumn(ModelSud::Colerg_AbgefuellteBiermenge);
    ui->tbJungbiermengeAbfuellen->setColumn(ModelSud::ColJungbiermengeAbfuellen);
    ui->tbSpeisemengeAbgefuellt->setColumn(ModelSud::ColSpeisemenge);
    ui->tbTemperaturJungbier->setColumn(ModelSud::ColTemperaturJungbier);
    ui->tbNebenkosten->setColumn(ModelSud::ColKostenWasserStrom);
    ui->tbSw->setColumn(ModelSud::ColSWIst);
    ui->tbEVG->setColumn(ModelSud::ColsEVG);
    ui->tbEVGRezept->setColumn(ModelSud::ColVergaerungsgrad);
    ui->tbGruenschlauchzeitpunkt->setColumn(ModelSud::ColGruenschlauchzeitpunkt);
    ui->tbAlkohol->setColumn(ModelSud::Colerg_Alkohol);
    ui->tbAlkoholRezept->setColumn(ModelSud::ColAlkohol);
    ui->tbSpundungsdruck->setColumn(ModelSud::ColSpundungsdruck);
    ui->tbTemperaturKarbonisierung->setColumn(ModelSud::ColTemperaturKarbonisierung);
    ui->tbWassserZuckerloesung->setColumn(ModelSud::ColVerschneidungAbfuellen);
    ui->tbKosten->setColumn(ModelSud::Colerg_Preis);
    ui->lblNebenkostenEinheit->setText(QLocale().currencySymbol());
    ui->lblKostenEinheit->setText(QLocale().currencySymbol() + "/" + tr("l"));

    mTimerWebViewUpdate.setSingleShot(true);
    connect(&mTimerWebViewUpdate, SIGNAL(timeout()), this, SLOT(updateWebView()), Qt::QueuedConnection);
    ui->webview->setHtmlFile("abfuelldaten");
    ui->webview->setPrintable(false);

    QPalette palette = ui->tbHelp->palette();
    palette.setBrush(QPalette::Base, palette.brush(QPalette::ToolTipBase));
    palette.setBrush(QPalette::Text, palette.brush(QPalette::ToolTipText));
    ui->tbHelp->setPalette(palette);

    gSettings->beginGroup("TabAbfuellen");

    ui->splitter->setSizes({500, 500});
    mDefaultSplitterState = ui->splitter->saveState();
    ui->splitter->restoreState(gSettings->value("splitterState").toByteArray());

    ui->splitterHelp->setStretchFactor(0, 1);
    ui->splitterHelp->setStretchFactor(1, 0);
    ui->splitterHelp->setSizes({90, 10});
    mDefaultSplitterHelpState = ui->splitterHelp->saveState();
    ui->splitterHelp->restoreState(gSettings->value("splitterHelpState").toByteArray());

    ui->tbZuckerFaktor->setValue(gSettings->value("ZuckerFaktor", 1.0).toDouble());
    ui->tbFlaschengroesse->setValue(gSettings->value("FlaschenGroesse", 0.5).toDouble());

    gSettings->endGroup();

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(focusChanged(QWidget*,QWidget*)));
    connect(bh, SIGNAL(modified()), this, SLOT(updateValues()));
    connect(bh, SIGNAL(discarded()), this, SLOT(sudLoaded()));
    connect(bh->sud(), SIGNAL(loadedChanged()), this, SLOT(sudLoaded()));
    connect(bh->sud(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)),
                    this, SLOT(sudDataChanged(const QModelIndex&)));
}

TabAbfuellen::~TabAbfuellen()
{
    delete ui;
}

void TabAbfuellen::saveSettings()
{
    gSettings->beginGroup("TabAbfuellen");
    gSettings->setValue("splitterState", ui->splitter->saveState());
    gSettings->setValue("splitterHelpState", ui->splitterHelp->saveState());
    gSettings->setValue("ZuckerFaktor", ui->tbZuckerFaktor->value());
    gSettings->setValue("FlaschenGroesse", ui->tbFlaschengroesse->value());
    gSettings->endGroup();
}

void TabAbfuellen::restoreView(bool full)
{
    if (full)
    {
        ui->splitter->restoreState(mDefaultSplitterState);
        ui->splitterHelp->restoreState(mDefaultSplitterHelpState);
    }
}

void TabAbfuellen::modulesChanged(Settings::Modules modules)
{
    if (modules.testFlag(Settings::ModulePreiskalkulation))
    {
        setVisibleModule(Settings::ModulePreiskalkulation,
                         {ui->tbKosten,
                          ui->lblKosten,
                          ui->lblKostenEinheit,
                          ui->tbNebenkosten,
                          ui->lblNebenkosten,
                          ui->lblNebenkostenEinheit,
                          ui->lblNebenkostenSpacer,
                          ui->lineKosten,
                          ui->lineKosten2});
    }
    if (modules.testFlag(Settings::ModuleSpeise))
    {
        setVisibleModule(Settings::ModuleSpeise,
                         {ui->tbSpeisemengeAbgefuellt,
                          ui->lblSpeisemengeAbgefuellt,
                          ui->lblSpeisemengeAbgefuelltEinheit,
                          ui->tbSpeisemengeGesamt,
                          ui->lblSpeisemengeGesamt,
                          ui->lblSpeisemengeGesamtEinheit,
                          ui->tbSpeisemengeFlasche,
                          ui->lblSpeisemengeFlasche,
                          ui->lblSpeisemengeFlascheEinheit});
    }
    updateValues();
}

void TabAbfuellen::focusChanged(QWidget *old, QWidget *now)
{
    if (old == ui->tbBemerkungAbfuellen)
    {
        QString bemerkung = ui->tbBemerkungAbfuellen->toPlainText().replace("<br>", "\n");
        if (bemerkung != bh->sud()->getBemerkungAbfuellen())
            bh->sud()->setBemerkungAbfuellen(bemerkung);
        ui->tbBemerkungAbfuellen->setHtml(bh->sud()->getBemerkungAbfuellen().replace("\n", "<br>"));
    }
    if (now == ui->tbBemerkungAbfuellen)
    {
        ui->tbBemerkungAbfuellen->setPlainText(bh->sud()->getBemerkungAbfuellen());
    }
    if (old == ui->tbBemerkungGaerung)
    {
        QString bemerkung = ui->tbBemerkungGaerung->toPlainText().replace("<br>", "\n");
        if (bemerkung != bh->sud()->getBemerkungGaerung())
            bh->sud()->setBemerkungGaerung(bemerkung);
        ui->tbBemerkungGaerung->setHtml(bh->sud()->getBemerkungGaerung().replace("\n", "<br>"));
    }
    if (now == ui->tbBemerkungGaerung)
    {
        ui->tbBemerkungGaerung->setPlainText(bh->sud()->getBemerkungGaerung());
    }

    if (now && now != ui->tbHelp && now != ui->splitterHelp)
        ui->tbHelp->setHtml(now->toolTip());
}

void TabAbfuellen::sudLoaded()
{
    checkEnabled();
    updateValues();
}

void TabAbfuellen::sudDataChanged(const QModelIndex& index)
{
    if (index.column() == ModelSud::ColStatus)
    {
        checkEnabled();
    }
}

void TabAbfuellen::onTabActivated()
{
    updateValues();
}

void TabAbfuellen::checkEnabled()
{
    Brauhelfer::SudStatus status = static_cast<Brauhelfer::SudStatus>(bh->sud()->getStatus());
    bool abgefuellt = status >= Brauhelfer::SudStatus::Abgefuellt && !gSettings->ForceEnabled;
    bool verbraucht = status >= Brauhelfer::SudStatus::Verbraucht && !gSettings->ForceEnabled;
    ui->tbAbfuelldatum->setReadOnly(abgefuellt);
    ui->tbAbfuelldatumZeit->setReadOnly(abgefuellt);
    ui->btnAbfuelldatumHeute->setVisible(!abgefuellt);
    ui->tbReifung->setReadOnly(verbraucht);
    ui->btnReifungHeute->setVisible(!abgefuellt);
    ui->tbReifezeit->setVisible(!verbraucht);
    ui->lblReifezeit->setVisible(!verbraucht);
    ui->lblReifezeitEinheit->setVisible(!verbraucht);
    ui->cbSchnellgaerprobeAktiv->setEnabled(!abgefuellt);
    ui->tbSWSchnellgaerprobe->setReadOnly(abgefuellt);
    ui->btnSWSchnellgaerprobe->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked() && !abgefuellt);
    ui->tbSWJungbier->setReadOnly(abgefuellt);
    ui->btnSWJungbier->setVisible(!abgefuellt);
    ui->tbTemperaturJungbier->setReadOnly(abgefuellt);
    ui->cbSpunden->setEnabled(!abgefuellt);
    ui->tbJungbiermengeAbfuellen->setReadOnly(abgefuellt);
    ui->tbBiermengeAbfuellen->setReadOnly(abgefuellt);
    ui->tbSpeisemengeAbgefuellt->setReadOnly(abgefuellt);
    ui->tbWassserZuckerloesung->setReadOnly(abgefuellt);
    ui->tbNebenkosten->setReadOnly(abgefuellt);
    ui->btnSudAbgefuellt->setEnabled(status == Brauhelfer::SudStatus::Gebraut && !gSettings->ForceEnabled);
    ui->btnSudVerbraucht->setEnabled(status == Brauhelfer::SudStatus::Abgefuellt && !gSettings->ForceEnabled);
}

void TabAbfuellen::updateValues()
{
    if (!isTabActive())
        return;

    for (DoubleSpinBoxSud *wdg : findChildren<DoubleSpinBoxSud*>())
        wdg->updateValue();
    for (SpinBoxSud *wdg : findChildren<SpinBoxSud*>())
        wdg->updateValue();

    QDateTime dt = bh->sud()->getAbfuelldatum();
    ui->tbAbfuelldatum->setMinimumDate(bh->sud()->getBraudatum().date());
    ui->tbAbfuelldatum->setDate(dt.isValid() ? dt.date() : QDateTime::currentDateTime().date());
    ui->tbAbfuelldatumZeit->setTime(dt.isValid() ? dt.time() : QDateTime::currentDateTime().time());
    ui->tbDauerHauptgaerung->setValue((int)bh->sud()->getBraudatum().daysTo(ui->tbAbfuelldatum->dateTime()));
    dt = bh->sud()->getReifungStart();
    ui->tbReifung->setMinimumDate(ui->tbAbfuelldatum->date());
    ui->tbReifung->setDate(dt.isValid() ? dt.date() : QDateTime::currentDateTime().date());

    ui->tbSWJungbierSoll->setValue(BierCalc::sreAusVergaerungsgrad(bh->sud()->getSWIst(), bh->sud()->getVergaerungsgrad()));
    ui->cbSchnellgaerprobeAktiv->setChecked(bh->sud()->getSchnellgaerprobeAktiv());
    ui->tbSWSchnellgaerprobe->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->lblSWSchnellgaerprobeEinheit->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->btnSWSchnellgaerprobe->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->tbGruenschlauchzeitpunkt->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->lblGruenschlauchzeitpunkt->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->lblGruenschlauchzeitpunktEinheit->setVisible(ui->cbSchnellgaerprobeAktiv->isChecked());
    ui->tbAlkoholGaerung->setValue(BierCalc::alkohol(bh->sud()->getSWIst(), bh->sud()->getSREIst()));

    ui->cbSpunden->setChecked(bh->sud()->getSpunden());
    ui->tbJungbierVerlust->setValue(bh->sud()->getWuerzemengeAnstellen() - bh->sud()->getJungbiermengeAbfuellen());

    ui->groupKarbonisierung->setVisible(!ui->cbSpunden->isChecked());
    double flascheFaktor = ui->tbFlaschengroesse->value() / bh->sud()->getJungbiermengeAbfuellen();

    // ModuleSpeise
    if (gSettings->module(Settings::ModuleSpeise))
    {
        ui->tbSpeisemengeGesamt->setValue((int)bh->sud()->getSpeiseAnteil());
        ui->tbSpeisemengeGesamt->setVisible(ui->tbSpeisemengeGesamt->value() > 0.0);
        ui->lblSpeisemengeGesamt->setVisible(ui->tbSpeisemengeGesamt->value() > 0.0);
        ui->lblSpeisemengeGesamtEinheit->setVisible(ui->tbSpeisemengeGesamt->value() > 0.0);
        ui->tbSpeisemengeFlasche->setValue(ui->tbSpeisemengeGesamt->value() * flascheFaktor);
        ui->tbSpeisemengeFlasche->setVisible(ui->tbSpeisemengeFlasche->value() > 0.0);
        ui->lblSpeisemengeFlasche->setVisible(ui->tbSpeisemengeFlasche->value() > 0.0);
        ui->lblSpeisemengeFlascheEinheit->setVisible(ui->tbSpeisemengeFlasche->value() > 0.0);
    }

    ui->tbZuckerGesamt->setValue((int)(bh->sud()->getZuckerAnteil() / ui->tbZuckerFaktor->value()));
    ui->tbZuckerFlasche->setValue(ui->tbZuckerGesamt->value() * flascheFaktor);
    ui->tbFlaschen->setValue(bh->sud()->geterg_AbgefuellteBiermenge() / ui->tbFlaschengroesse->value());
    ui->tbKonzentrationZuckerloesung->setValue(ui->tbZuckerGesamt->value() / ui->tbWassserZuckerloesung->value());
    bool hasZucker = ui->tbZuckerGesamt->value() > 0.0;
    ui->tbZuckerGesamt->setVisible(hasZucker);
    ui->lblZuckerGesamt->setVisible(hasZucker);
    ui->lblZuckerGesamtEinheit->setVisible(hasZucker);
    ui->tbZuckerFaktor->setVisible(hasZucker);
    ui->lblZuckerFaktor->setVisible(hasZucker);
    ui->tbZuckerFlasche->setVisible(hasZucker);
    ui->lblZuckerFlasche->setVisible(hasZucker);
    ui->lblZuckerFlascheEinheit->setVisible(hasZucker);
    ui->lblWassserZuckerloesung->setVisible(hasZucker);
    ui->tbWassserZuckerloesung->setVisible(hasZucker);
    ui->tbWassserZuckerloesungEinheit->setVisible(hasZucker);
    bool hasZuckerLoesung = ui->tbWassserZuckerloesung->value() > 0.0 && hasZucker;
    ui->lblKonzentrationZuckerloesung->setVisible(hasZuckerLoesung);
    ui->tbKonzentrationZuckerloesung->setVisible(hasZuckerLoesung);
    ui->tbKonzentrationZuckerloesungEinheit->setVisible(hasZuckerLoesung);

    if (!ui->tbBemerkungAbfuellen->hasFocus())
        ui->tbBemerkungAbfuellen->setHtml(bh->sud()->getBemerkungAbfuellen().replace("\n", "<br>"));
    if (!ui->tbBemerkungGaerung->hasFocus())
        ui->tbBemerkungGaerung->setHtml(bh->sud()->getBemerkungGaerung().replace("\n", "<br>"));

    mTimerWebViewUpdate.start(200);
}

void TabAbfuellen::updateWebView()
{
    TemplateTags::render(ui->webview, TemplateTags::TagAll, bh->sud()->row());
}

void TabAbfuellen::on_tbAbfuelldatum_dateChanged(const QDate &date)
{
    if (ui->tbAbfuelldatum->hasFocus())
        bh->sud()->setAbfuelldatum(QDateTime(date, ui->tbAbfuelldatumZeit->time()));
}

void TabAbfuellen::on_tbAbfuelldatumZeit_timeChanged(const QTime &time)
{
    if (ui->tbAbfuelldatumZeit->hasFocus())
        bh->sud()->setAbfuelldatum(QDateTime(ui->tbAbfuelldatum->date(), time));
}

void TabAbfuellen::on_btnAbfuelldatumHeute_clicked()
{
    bh->sud()->setAbfuelldatum(QDateTime());
}

void TabAbfuellen::on_tbReifung_dateChanged(const QDate &date)
{
    if (ui->tbReifung->hasFocus())
        bh->sud()->setReifungStart(QDateTime(date, QTime()));
}

void TabAbfuellen::on_btnReifungHeute_clicked()
{
    bh->sud()->setReifungStart(QDateTime());
}

void TabAbfuellen::on_cbSchnellgaerprobeAktiv_clicked(bool checked)
{
    bh->sud()->setSchnellgaerprobeAktiv(checked);
}

void TabAbfuellen::on_btnSWSchnellgaerprobe_clicked()
{
    WidgetDecorator::suspendValueChanged(true);
    DlgRestextrakt dlg(ui->tbSWSchnellgaerprobe->value(),
                       bh->sud()->getSWIst(),
                       ui->tbTemperaturJungbier->value(),
                       QDateTime(),
                       this);
    int dlgRet = dlg.exec();
    WidgetDecorator::suspendValueChanged(false);
    if (dlgRet == QDialog::Accepted)
    {
        bh->sud()->setTemperaturJungbier(dlg.temperatur());
        bh->sud()->setSWSchnellgaerprobe(dlg.value());
    }
}

void TabAbfuellen::on_btnSWJungbier_clicked()
{
    WidgetDecorator::suspendValueChanged(true);
    DlgRestextrakt dlg(ui->tbSWJungbier->value(),
                       bh->sud()->getSWIst(),
                       ui->tbTemperaturJungbier->value(),
                       QDateTime(),
                       this);
    int dlgRet = dlg.exec();
    WidgetDecorator::suspendValueChanged(false);
    if (dlgRet == QDialog::Accepted)
    {
        bh->sud()->setTemperaturJungbier(dlg.temperatur());
        bh->sud()->setSWJungbier(dlg.value());
    }
}

void TabAbfuellen::on_cbSpunden_clicked(bool checked)
{
    bh->sud()->setSpunden(checked);
}

void TabAbfuellen::on_tbZuckerFaktor_valueChanged(double)
{
    if (ui->tbZuckerFaktor->hasFocus())
        updateValues();
}

void TabAbfuellen::on_tbFlaschengroesse_valueChanged(double)
{
    if (ui->tbFlaschengroesse->hasFocus())
        updateValues();
}

void TabAbfuellen::on_btnSudAbgefuellt_clicked()
{
    if (!bh->sud()->getAbfuellenBereitZutaten())
    {
        QMessageBox::warning(this, tr("Zutaten Gärung"),
                             tr("Es wurden noch nicht alle Zutaten für die Gärung zugegeben oder entnommen."));
        return;
    }

    if (bh->sud()->getSchnellgaerprobeAktiv())
    {
        if (bh->sud()->getSWJungbier() > bh->sud()->getGruenschlauchzeitpunkt())
        {
            QMessageBox::warning(this, tr("Grünschlauchzeitpunkt nicht erreicht"),
                                 tr("Der Grünschlauchzeitpunkt wurde noch nicht erreicht."));
            return;
        }
        else if (bh->sud()->getSWJungbier() < bh->sud()->getSWSchnellgaerprobe())
        {
            QMessageBox::warning(this, tr("Schnellgärprobe"),
                                 tr("Die Stammwürze des Jungbiers liegt tiefer als die der Schnellgärprobe."));
            return;
        }
    }

    QDateTime dt(ui->tbAbfuelldatum->date(), ui->tbAbfuelldatumZeit->time());
    QString dtStr = QLocale().toString(dt, QLocale::ShortFormat);
    if (QMessageBox::question(this, tr("Sud als abgefüllt markieren?"),
                                    tr("Soll der Sud als abgefüllt markiert werden?\n\nAbfülldatum: %1").arg(dtStr),
                                    QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes)
        return;

    bh->sud()->setAbfuelldatum(dt);
    bh->sud()->setReifungStart(ui->tbReifung->dateTime());
    bh->sud()->setStatus(static_cast<int>(Brauhelfer::SudStatus::Abgefuellt));

    QMap<int, QVariant> values({{ModelNachgaerverlauf::ColSudID, bh->sud()->id()},
                                {ModelNachgaerverlauf::ColZeitstempel, bh->sud()->getAbfuelldatum()},
                                {ModelNachgaerverlauf::ColDruck, 0.0},
                                {ModelNachgaerverlauf::ColTemp, bh->sud()->getTemperaturJungbier()}});
    if (bh->sud()->modelNachgaerverlauf()->rowCount() == 0)
        bh->sud()->modelNachgaerverlauf()->append(values);
}

void TabAbfuellen::on_btnSudVerbraucht_clicked()
{
    bh->sud()->setStatus(static_cast<int>(Brauhelfer::SudStatus::Verbraucht));
}
