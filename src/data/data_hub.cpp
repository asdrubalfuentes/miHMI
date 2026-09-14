#include "data_hub.h"
#include "hist_log.h"

DataHub &DataHub::instance() {
	static DataHub inst;
	return inst;
}

void DataHub::begin() {
	if (primary_) primary_->begin();
	if (backup_)  backup_->begin();
	active_ = primary_ ? primary_ : backup_;
}

SrcHealth DataHub::healthOf(DataSource *s) const {
	if (!s) return SrcHealth::Down;
	uint32_t age = millis() - s->lastOkMs();
	if (!s->isHealthy() || age > 15000) return SrcHealth::Down;
	if (age > 4000) return SrcHealth::Stale;
	return SrcHealth::Ok;
}

SrcHealth DataHub::primaryHealth() const { return healthOf(primary_); }
SrcHealth DataHub::backupHealth()  const { return healthOf(backup_);  }

const char *DataHub::activeSourceName() const {
	return active_ ? active_->name() : "--";
}

void DataHub::enqueue(CmdType t, uint8_t well) {
	if (qCount_ >= QN) return;                 /* cola llena: descartar */
	uint8_t tail = (qHead_ + qCount_) % QN;
	queue_[tail].type = t;
	queue_[tail].well = well;
	qCount_++;
}

void DataHub::tick() {
	/* Sondear SIEMPRE ambas fuentes: si solo se sondea la activa, una primaria
	 * caida nunca vuelve a conectar (deadlock de failover). */
	if (primary_)                        primary_->poll();
	if (backup_ && backup_ != primary_)  backup_->poll();

	/* Seleccion de fuente activa con failover */
	DataSource *want = nullptr;
	if (primary_ && healthOf(primary_) != SrcHealth::Down)      want = primary_;
	else if (backup_ && healthOf(backup_) != SrcHealth::Down)   want = backup_;
	else                                                        want = primary_ ? primary_ : backup_;
	active_ = want;
	if (!active_) return;

	/* Vaciar la cola de comandos contra la fuente activa */
	while (qCount_) {
		Command c = queue_[qHead_];
		if (active_->sendCommand(c)) txOk_++; else txErr_++;
		qHead_ = (qHead_ + 1) % QN;
		qCount_--;
	}

	data_ = active_->latest;
	histlog::tick(data_);   /* cierre de dia/mes (hora local) + historico persistido */
}
