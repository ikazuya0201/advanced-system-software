# M5Stack側の実装

# タスク
- ButtonTask
	-	各ボタンの状態を監視する.
	- ボタンが押されたら変数の状態を更新する.
	- 優先度は最も低い.(正直いつでもいいので)
- IMUTask
	- 内臓のジャイロセンサーでデータが計測し, 加速度から速度を求める.
	- 状態を更新してデータをサーバータスクに送る.
	- データの同期はFreeRTOSのデータ転送機能であるQueueを使って通信している.
	- 優先度は最高.(WOWOW)
- サーバータスク
	- Webサーバーを立ててそこに各タスクで同期されたデータをjson形式でWebサーバーに置く.
	- 優先度はボタンより上.

# 処理の順序
1. M5Stackを起動する.
1. Wifiアクセスポイントとして起動する.
1. 全てのタスクを動作させる.
