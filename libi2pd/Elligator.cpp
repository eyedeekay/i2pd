#include "Crypto.h"
#include "Elligator.h"

namespace i2p
{
namespace crypto
{
	
	Elligator2::Elligator2 ()
	{
		// TODO: share with Ed22519
		p = BN_new ();
		// 2^255-19
		BN_set_bit (p, 255); // 2^255
		BN_sub_word (p, 19);
		p38 = BN_dup (p); BN_add_word (p38, 3); BN_div_word (p38, 8); // (p+3)/8
		p12 = BN_dup (p); BN_sub_word (p12, 1); BN_div_word (p12, 2); // (p-1)/2
		p14 = BN_dup (p); BN_sub_word (p14, 1); BN_div_word (p14, 4); // (p-1)/4

		auto A = BN_new (); BN_set_word (A, 486662);
		nA = BN_new (); BN_sub  (nA, p, A);

		BN_CTX * ctx = BN_CTX_new ();	
		// calculate sqrt(-1)		
		sqrtn1 = BN_new ();
		BN_set_word (sqrtn1, 2);	
		BN_mod_exp (sqrtn1, sqrtn1, p14, p, ctx); // 2^((p-1)/4
		
		u = BN_new (); BN_set_word (u, 2);
		iu = BN_new (); BN_mod_inverse (iu, u, p, ctx);	
		//printf ("%s\n", BN_bn2hex (iu));

		BN_CTX_free (ctx);
	}

	Elligator2::~Elligator2 ()
	{
		BN_free (p); BN_free (p38); BN_free (p12); BN_free (p14);
		BN_free (sqrtn1); BN_free (A); BN_free (nA); 
		BN_free (u); BN_free (iu); 
	}

	void Elligator2::Encode (const uint8_t * key, uint8_t * encoded) const
	{
		BN_CTX * ctx = BN_CTX_new ();
		BN_CTX_start (ctx);

		BIGNUM * x = BN_CTX_get (ctx); BN_bin2bn (key, 32, x);
		BIGNUM * xA = BN_CTX_get (ctx); BN_add (xA, x, A); // x + A
		BN_sub (xA, p, xA); // p - (x + A)

		BIGNUM * r = BN_CTX_get (ctx);
		BN_mod_inverse (r, xA, p, ctx);	
		BN_mod_mul (r, r, x, p, ctx);	
		BN_mod_mul (r, r, iu, p, ctx);	
		
		SquareRoot (r, r, ctx);
		bn2buf (r, encoded, 32);

		BN_CTX_end (ctx);	
		BN_CTX_free (ctx);
	}

	void Elligator2::SquareRoot (const BIGNUM * x, BIGNUM * r, BN_CTX * ctx) const
	{
		BIGNUM * t = BN_CTX_get (ctx);
		BN_mod_exp (t, x, p14, p, ctx); // t = x^((p-1)/4)
		BN_mod_exp (r, x, p38, p, ctx); // r = x^((p+3)/8)
		BN_add_word (t, 1);

		if (!BN_cmp (t, p))
			BN_mod_mul (r, r, sqrtn1, p, ctx);	

		if (BN_cmp (r, p12) > 0) // r > (p-1)/2
			BN_sub (r, p, r);
	}	
	
	static std::unique_ptr<Elligator2> g_Elligator;
	std::unique_ptr<Elligator2>& GetElligator ()
	{
		if (!g_Elligator)
		{
			auto el = new Elligator2();
			if (!g_Elligator) // make sure it was not created already
				g_Elligator.reset (el);
			else
				delete el;
		}
		return g_Elligator;
	}
}
}

