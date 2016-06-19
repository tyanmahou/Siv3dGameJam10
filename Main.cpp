
# include <Siv3D.hpp>


const int g_pixelSize = 64;

using HashData = Grid<bool>;
//ハッシュ生成
//画像の類似度の計算に使用
void CreateAverageHash(const Image& image, HashData& grid)
{

	//グレイスケール作成
	Image grayImg = image.grayscaled();

		grayImg.scale(g_pixelSize, g_pixelSize);

	//色の平均値計算
	int sumV=0;
	for (int y = 0; y < grayImg.height; ++y)
	{
		for (int x = 0; x < grayImg.width; ++x)
		{
			sumV+= grayImg[y][x].r;
		}
	}
	float aveV = static_cast<float>(sumV) / grayImg.num_pixels;

	for (int y = 0; y < grayImg.height; ++y)
	{
		for (int x = 0; x < grayImg.width; ++x)
		{
			if (aveV > grayImg[y][x].r)
				grid[y][x] = true;
			else
				grid[y][x] = false;
		}
	}

}

//スコアの計算
int ScoreCal(HashData& a, HashData&b)
{
	int matchCount=0;
	for (int y = 0; y < a.height;++y)
		for (int x = 0; x < a.width; ++x) 
		{
			if (a[y][x] == b[y][x])
				++matchCount;
		}


	int score= static_cast<int>(static_cast<float>(matchCount) / a.num_elements() * 100.0);
	return score;

}

//シーン管理
enum class State
{
	Authorize,
	Title,
	Play,
	Result,
};
void Main()
{
	Window::SetTitle(L"Siv3Dくん版深夜のお絵かき10秒一本勝負");

	//ツイッター連携
	const String API_key = L"ここは自分でやってね";

	const String API_secret = L"ここは自分でやってね";

	TwitterClient twitter(API_key, API_secret);
	twitter.openTokenPage();

	GUI guiAuth(GUIStyle::Default);
	guiAuth.setTitle(L"PIN の入力");
	guiAuth.addln(L"PIN", GUITextField::Create(7));
	guiAuth.add(L"auth", GUIButton::Create(L"認証"));
	guiAuth.setPos(40, 40);

	//GUIの生成
	GUI gui(GUIStyle::Default);
	gui.setTitle(L"色の選択");
	gui.add(L"color", GUIColorPalette::Create(Palette::Lightseagreen));
	gui.add(L"hr", GUIHorizontalLine::Create(1));
	gui.add(L"size", GUISlider::Create(1.0, 50.0, 15.0, 200));
	gui.setPos(400, 0);

	//フォント
	const Font font(10);
	const Font font2(30);

	//bgm
	const Sound sound(L"bgm.wav");

	const Texture logo(L"logo.png");
	//ベースのイラスト
	const Image baseImg(L"Siv3D-kun.png");
	const Texture baseTex(baseImg);

	//ベースのハッシュ
	HashData baseHash(g_pixelSize, g_pixelSize,0);

	CreateAverageHash(baseImg, baseHash);

	//自分の描くほう
	Image canvas({350,480}, Palette::White);

	DynamicTexture myTex(canvas);

	HashData canvasHash(g_pixelSize, g_pixelSize,0);

	//スコア
	int score = 0;

	State state = State::Authorize;


	 const Rect button1(20, 320, 200, 50);
	 const Rect button2(20, 390, 200, 50);
	 const RoundRect messageBox({ 0,300 }, { 240,150 }, 10);

	while (System::Update())
	{

		myTex.draw();

		const float alpha =
			state == State::Title ? 1 :
			state == State::Play ? 0.1 : 0;
		baseTex.draw(ColorF(1, alpha));

		switch (state)
		{
		case State::Authorize:
			//update
			if (guiAuth.button(L"auth").pushed && twitter.verifyPIN(guiAuth.textField(L"PIN").text))
			{
				Println(L"認証されました。");
				guiAuth.release();
				state = State::Title;
			}
			//draw
			font(L"Twitter認証してね").draw(100, 300, Palette::Black);
			break;
		case State::Title:
		{

			//update
			if (button1.leftClicked)
			{
				sound.play();
				ClearPrint();
				canvas.fill(Palette::White);
				myTex.fill(canvas);
				state = State::Play;
			}
			if (button2.leftClicked)
			{
				System::Exit();
			}

			//draw

			logo.scale(0.5).draw(0,100);

			messageBox.draw(ColorF(0.2, 0.6, 0.4, 0.9));

			button1.drawFrame(1, 1, Palette::White);
			if (button1.mouseOver)
				button1.draw(AlphaF(0.3));
			font(L"ゲームスタート").drawCenter(button1.center, Palette::White);

			button2.drawFrame(1, 1, Palette::White);
			if (button2.mouseOver)
				button2.draw(AlphaF(0.3));
			font(L"終了").drawCenter(button2.center, Palette::White);


		}
			break;
		case State::Play:
		{
			//update	
			if (Input::MouseL.pressed)
			{
				const Point from = Input::MouseL.clicked ? Mouse::Pos() : Mouse::PreviousPos();

				Line(from, Mouse::Pos()).overwrite(canvas, gui.slider(L"size").valueInt, gui.colorPalette(L"color").color);

				myTex.fill(canvas);
			}

			if (sound.streamPosSec() >= 10)
			{

				CreateAverageHash(canvas, canvasHash);
				score = ScoreCal(baseHash, canvasHash);
				state = State::Result;
			}
			const Vec2 pos(500, 400);
			//draw
			font2(0_dp, 10.0 - sound.streamPosSec()).drawCenter(pos);
			const double angle = Radians(static_cast<int>(sound.streamPosSec()*360)%360);
			Circle(pos,30).drawArc(0, angle);
		}
			break;
		case State::Result:
		{
			//update	
			if (button1.leftClicked)
			{
				sound.stop();
				state = State::Title;
			}
			if (button2.leftClicked)
			{
				bool result = twitter.tweetWithMedia(L"今回のスコア："+Format(score) + L"点w\n#しぶ3Dくん版深夜のお絵かき10秒一本勝負", canvas);
				if (result)
					Println(L"ツイートされました。");
				sound.stop();
				state = State::Title;
			}
			//draw	
			font(L"今回のスコア:\n").draw(400, 380);
			font2(Format(score) + L"点w").draw(450, 410);
			messageBox.draw(ColorF(0.2, 0.6, 0.4, 0.9));

			button1.drawFrame(1, 1, Palette::White);
			if (button1.mouseOver)
				button1.draw(AlphaF(0.3));
			font(L"戻る").drawCenter(button1.center, Palette::White);

			button2.drawFrame(1, 1, Palette::White);
			if (button2.mouseOver)
				button2.draw(AlphaF(0.3));
			font(L"結果をツイートして戻る").drawCenter(button2.center, Palette::White);
			break;
		}
		default:
			break;
		}



	}
}
