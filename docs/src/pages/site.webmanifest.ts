const { BASE_URL } = import.meta.env;

const webmanifest = {
  name: "Live Background Removal Lite",
  short_name: "Live BR Lite",
  icons: [
    {
      src: `${BASE_URL}/web-app-manifest-192x192.png`,
      sizes: "192x192",
      type: "image/png",
      purpose: "maskable"
    },
    {
      src: `${BASE_URL}/web-app-manifest-512x512.png`,
      sizes: "512x512",
      type: "image/png",
      purpose: "maskable"
    }
  ],
  theme_color: "#87ceeb",
  background_color: "#000000",
  display: "standalone"
};

export async function GET() {
  return new Response(JSON.stringify(webmanifest), {
    status: 200,
    headers: {
      "Content-Type": "application/manifest+json",
    },
  });
}
