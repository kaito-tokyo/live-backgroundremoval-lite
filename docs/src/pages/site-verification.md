---
layout: ../layouts/MarkdownLayout.astro
pathname: site-verification
lang: en
title: Live Background Removal Lite – Site Verification Guide
description: "Learn how to verify your website with search engines (Google, Bing) and other services for the Live Background Removal Lite documentation site."
---

# Site Verification Guide

This guide explains how to verify ownership of the Live Background Removal Lite documentation website with various search engines and web services.

---

## What is Site Verification?

Site verification is the process of proving to search engines and web services that you own or control a website. This is typically required to:

- Access webmaster tools and analytics
- Submit sitemaps to search engines
- Monitor search performance
- Identify and fix indexing issues
- Receive important notifications about your site

---

## Verification Methods

There are several common methods to verify site ownership:

### 1. HTML File Upload

Upload a verification file provided by the service to your website's root directory.

**Steps:**

1. Download the verification file from the service (e.g., `google[VERIFICATION_CODE].html`)
2. Place the file in the `docs/public/` directory
3. The file will be accessible at `https://kaito-tokyo.github.io/live-backgroundremoval-lite/google[VERIFICATION_CODE].html`
4. Verify ownership through the service's interface

### 2. HTML Meta Tag

Add a meta tag to the `<head>` section of your website.

**Steps:**

1. Get the meta tag from the service (e.g., `<meta name="google-site-verification" content="..." />`)
2. Add it to `docs/src/layouts/Layout.astro` in the `<head>` section (after the viewport meta tag and before the title tag)
3. Deploy the changes
4. Verify ownership through the service's interface

**Note:** Meta tags added to `Layout.astro` will be included on all pages of the site, which is appropriate for site-wide verification.

### 3. DNS Record (TXT)

Add a TXT record to your domain's DNS configuration.

**Note:** This method is only available if you control the DNS settings for your custom domain. For GitHub Pages sites using `github.io` domains, this method is not applicable.

---

## Google Search Console Verification

Google Search Console allows you to monitor and maintain your site's presence in Google Search results.

### Method 1: HTML File Upload

1. Go to [Google Search Console](https://search.google.com/search-console)
2. Add your property: `https://kaito-tokyo.github.io/live-backgroundremoval-lite/`
3. Choose "HTML file upload" as the verification method
4. Download the verification file
5. Add the file to `docs/public/` directory
6. Commit and push the changes
7. Wait for GitHub Pages to deploy
8. Click "Verify" in Google Search Console

### Method 2: HTML Meta Tag

1. Go to [Google Search Console](https://search.google.com/search-console)
2. Add your property
3. Choose "HTML tag" as the verification method
4. Copy the meta tag provided
5. Edit `docs/src/layouts/Layout.astro`:

```astro
<!-- In docs/src/layouts/Layout.astro -->
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width" />
  <!-- Add Google verification tag here, after viewport and before title -->
  <meta name="google-site-verification" content="YOUR_VERIFICATION_CODE" />
  {
    import.meta.env.PROD && (
      <meta http-equiv="Content-Security-Policy" content="..." />
    )
  }
  <title>{title}</title>
  <!-- Rest of head content -->
</head>
```

6. Commit, push, and deploy
7. Click "Verify" in Google Search Console

---

## Bing Webmaster Tools Verification

Bing Webmaster Tools provides insights and tools for your site's presence in Bing search results.

### Method 1: HTML File Upload

1. Go to [Bing Webmaster Tools](https://www.bing.com/webmasters)
2. Add your site: `https://kaito-tokyo.github.io/live-backgroundremoval-lite/`
3. Choose "HTML file upload" verification
4. Download the BingSiteAuth.xml file
5. Add the file to `docs/public/` directory
6. Deploy and verify

### Method 2: HTML Meta Tag

1. Go to [Bing Webmaster Tools](https://www.bing.com/webmasters)
2. Add your site
3. Choose "HTML Meta Tag" verification
4. Copy the meta tag
5. Add it to `docs/src/layouts/Layout.astro` in the `<head>` section (after the viewport meta tag):

```astro
<!-- In docs/src/layouts/Layout.astro, in the <head> section -->
<meta name="msvalidate.01" content="YOUR_VERIFICATION_CODE" />
```

6. Commit, push, and wait for deployment
7. Click "Verify" in Bing Webmaster Tools

---

## Yandex Webmaster Verification

Yandex is a popular search engine, especially in Russia and Eastern Europe.

### HTML Meta Tag Method

1. Go to [Yandex Webmaster](https://webmaster.yandex.com/)
2. Add your site
3. Choose meta tag verification
4. Add the verification meta tag to `docs/src/layouts/Layout.astro` in the `<head>` section:

```astro
<!-- In docs/src/layouts/Layout.astro, in the <head> section -->
<meta name="yandex-verification" content="YOUR_VERIFICATION_CODE" />
```

5. Commit, push, and wait for deployment
6. Click "Verify" in Yandex Webmaster

---

## GitHub Pages Specific Considerations

Since this site is hosted on GitHub Pages, there are some important considerations:

### Deployment Process

1. Changes must be committed to the repository
2. GitHub Actions will build and deploy the site
3. Wait for the deployment to complete (usually 1-5 minutes)
4. Verification files or meta tags will then be accessible

### File Location

- HTML verification files go in `docs/public/`
- Meta tags go in `docs/src/layouts/Layout.astro`
- Files in `docs/public/` are served at the root URL path

### Automated Sitemap

The site uses `@astrojs/sitemap` integration, which automatically generates a sitemap at:

```
https://kaito-tokyo.github.io/live-backgroundremoval-lite/sitemap-index.xml
```

This sitemap is already referenced in `robots.txt` and can be submitted to search engines after verification.

---

## Post-Verification Steps

After successfully verifying your site:

### 1. Submit Sitemap

Submit your sitemap to search engines:

- **Google Search Console:** Sitemaps → Add new sitemap → Enter `sitemap-index.xml`
- **Bing Webmaster Tools:** Sitemaps → Submit Sitemap → Enter full sitemap URL

### 2. Monitor Performance

- Check the "Performance" section in Google Search Console to see search queries and impressions
- Review the "URL Inspection" tool to ensure pages are being indexed correctly

### 3. Check for Issues

- Review "Coverage" reports to identify any indexing problems
- Fix any mobile usability issues
- Address any security issues or manual actions

### 4. Configure Settings

- Set your preferred domain (with or without www, if applicable)
- Configure crawl rate settings if needed
- Set up email notifications for critical issues

---

## Troubleshooting

### Verification Failed

**Problem:** Verification fails even after adding the file or meta tag.

**Solutions:**

1. Clear your browser cache
2. Wait for GitHub Pages deployment to complete (check Actions tab on GitHub)
3. Verify the file/tag is accessible by visiting the URL directly
4. Check for typos in the verification code
5. Ensure the file is in the correct location (`docs/public/`)

### File Not Found (404)

**Problem:** Verification file returns 404 error.

**Solutions:**

1. Ensure the file is in `docs/public/` directory (not a subdirectory)
2. Check that the file was committed and pushed to the repository
3. Wait for GitHub Actions to complete the deployment
4. Verify the build succeeded in the Actions tab

### Changes Not Reflecting

**Problem:** Meta tag or file changes are not visible on the live site.

**Solutions:**

1. Check the GitHub Actions workflow status
2. Clear browser cache and hard refresh (Ctrl+Shift+R or Cmd+Shift+R)
3. Verify changes were committed to the correct branch (usually `main`)
4. Wait a few minutes for CDN caches to clear

---

## Best Practices

1. **Use Multiple Verification Methods:** Consider adding both HTML file and meta tag for redundancy
2. **Document Verification Codes:** Keep a record of which codes belong to which services
3. **Regular Monitoring:** Check webmaster tools regularly for issues or opportunities
4. **Sitemap Updates:** The sitemap is automatically generated, but verify it's complete after adding new pages
5. **Security:** Treat verification codes as sensitive information; don't share them publicly

---

## Additional Resources

- [Google Search Console Help](https://support.google.com/webmasters)
- [Bing Webmaster Tools Help](https://www.bing.com/webmasters/help/webmaster-guidelines-30fba23a)
- [Yandex Webmaster Help](https://yandex.com/support/webmaster/)
- [GitHub Pages Documentation](https://docs.github.com/en/pages)
- [Astro Sitemap Integration](https://docs.astro.build/en/guides/integrations-guide/sitemap/)

---

## Summary

Site verification is an essential step for maintaining a healthy web presence. For the Live Background Removal Lite documentation site:

1. Choose your verification method (HTML file or meta tag)
2. Add verification assets to the appropriate location
3. Deploy via GitHub Pages
4. Complete verification in the respective webmaster tool
5. Submit your sitemap
6. Monitor and maintain your site's search presence

For questions or issues related to site verification for this project, please open an issue on the [GitHub repository](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues).
