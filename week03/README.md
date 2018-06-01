# 進捗

* [スケルトン](https://twitter.com/mathbbN/status/999159526162026496)
* initializeでNUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORTを指定すると全身のスケルトンが取れない
* drawTrackedSkeleton関数で関節同士の線分を引く
* setSkeletonImage関数のなかでローカル変数だったskeletonをヘッダーに切り分ける

# 資料の言葉が足りないところを補う。

辞書データ数(例:N=20)を予め定義する。
それから、Nフレーム保存する。

[誤]それから、連続したNフレームを保存する。
[正]それから、キーが押されるたびに初期化した辞書に1フレームずつ格納する。
