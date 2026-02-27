# Android_Development_backend


## Практическая работа №10 "Android background service."

### Ссылка на репозиторий с Android: https://github.com/seg0ga/AndroidDevelopment_sibsutis
### Цель
1. Реализовать в приложении Android сервис работы в фоновом режиме.✅
2. Реализовать получение данных и передачу на backend-сервер след. данные о смартфоне:
   1. При помощи класса Telephony получаем информацию о сетях мобильной связи:
      1. CellInfoLte: CellIdentityLte, CellSignalStrengthLte;
         1. CellIdentityLte: Band, CellIdentity, EARFCN, MCC, MNC, PCI, TAC;✅
         2. CellSignalStrengthLte: ASU Level, CQI, RSRP, RSRQ, RSSI, RSSNR, Timing Advance;✅
      2. CellInfoGsm: CellIdentityGSM, CellSignalStrengthGsm;
         1. CellIdentityGSM: CellIdentity, BSIC, ARFCN, LAC, MCC, MNC, PSC;✅
         2. CellSignalStrengthGsm: Dbm, RSSI, Timing Advance;✅
      3. CellInfoNr: CellwIdentityNr, CellSignalStrengthNr
         1. CellIdentityNr: Band, NCI, PCI, Nrargcn, TAC, MCC, MNC;✅
         2. CellSignalStrengthNr: SS-RSRP, SS-RSRQ, SS-SINR, Timing Advance;✅
   2. Данные о местоположении смартфона (см. практику №6):
      1. Latitude;✅
      2. Longitude;✅
      3. Altitude;✅
      4. Current Time;✅
      5. Accurace - точность вычисления местоположения✅
   3. Информацию о сетевом трафике смартфона:
      1. Информация об общем количестве переданных данных;✅
      2. Информация о ТОП приложений (входящих в 2-сигма по потреблению трафика), потребляющих интернет-трафик смартфона.⚠️ (В процессе)

<img width="301" height="595" alt="image" src="https://github.com/user-attachments/assets/0a5286d2-8409-4feb-a576-6aaecec87b06" />

<img width="1280" height="779" alt="image" src="https://github.com/user-attachments/assets/45ecd7aa-d62a-4244-8545-7303c8e4f2b6" />